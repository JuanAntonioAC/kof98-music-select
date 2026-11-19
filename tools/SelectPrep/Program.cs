using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text;
using System.Threading.Tasks;

internal static class Program {
    private const int Rate = 44100;
    private static string s_ffmpeg;

    private static int Main(string[] args) {
        var opt = new Dictionary<string, string>();
        for (int i = 0; i + 1 < args.Length; i += 2) opt[args[i].TrimStart('-')] = args[i + 1];
        s_ffmpeg = opt["ffmpeg"];
        string src = opt["src"];
        string dst = opt["out"];
        Directory.CreateDirectory(Path.Combine(dst, "kof98"));
        Directory.CreateDirectory(Path.Combine(dst, "kof2002"));
        SelfTest();
        var files = new List<string>();
        foreach (string f in Directory.GetFiles(src, "*.mp4")) {
            if (Path.GetFileName(f).IndexOf(" (1)", StringComparison.OrdinalIgnoreCase) >= 0) continue;
            files.Add(f);
        }
        int done = 0;
        Parallel.ForEach(files, new ParallelOptions { MaxDegreeOfParallelism = 2 }, f => {
            string stem = Stem(Path.GetFileName(f));
            float[] pcm = Decode(f);
            int n = pcm.Length / 2;
            int loop, end;
            string how = Cut(pcm, n, out loop, out end, out float[] cut);
            Encode(cut, Path.Combine(dst, "kof98", stem + ".ogg"), "-c:a libvorbis -q:a 5");
            Encode(cut, Path.Combine(dst, "kof2002", stem + ".ogg"), "-ar 48000 -c:a libvorbis -q:a 4");
            string sec = (loop / (double)Rate).ToString("0.000000", CultureInfo.InvariantCulture);
            File.WriteAllText(Path.Combine(dst, "kof98", stem + ".loop"), sec);
            File.WriteAllText(Path.Combine(dst, "kof2002", stem + ".loop"), sec);
            int k = System.Threading.Interlocked.Increment(ref done);
            Console.WriteLine($"[{k}/{files.Count}] {stem,-6} {how,-8} loop {loop / (double)Rate,7:0.000}s  dura {end / (double)Rate,7:0.000}s  de {n / (double)Rate,7:0.000}s  {Path.GetFileName(f)}");
        });
        return 0;
    }

    private static string Stem(string name) {
        if (name.IndexOf("2000", StringComparison.Ordinal) >= 0) return "SEL00";
        if (name.IndexOf("2001", StringComparison.Ordinal) >= 0) return "SEL01";
        if (name.IndexOf("Roulette", StringComparison.OrdinalIgnoreCase) >= 0) return "SEL96";
        if (name.StartsWith("Select", StringComparison.OrdinalIgnoreCase)) return "SEL02";
        if (name.IndexOf("94", StringComparison.Ordinal) >= 0) return "SEL94";
        if (name.IndexOf("95", StringComparison.Ordinal) >= 0) return "SEL95";
        if (name.IndexOf("97", StringComparison.Ordinal) >= 0) return "SEL97";
        if (name.IndexOf("98", StringComparison.Ordinal) >= 0) return "SEL98";
        if (name.IndexOf("99", StringComparison.Ordinal) >= 0) return "SEL99";
        return "SELXX";
    }

    private static string Cut(float[] st, int n, out int loop, out int end, out float[] cut) {
        int a, b;
        double score;
        bool period = FindPeriod(st, n, out a, out b, out score);
        if (!period) Trim(st, n, out a, out b);
        int len = Math.Max(1, b - a);
        cut = new float[len * 2];
        Array.Copy(st, a * 2, cut, 0, cut.Length);
        Crossfade(cut, len, period ? Rate / 80 : Rate / 16);
        loop = Rate / 1000;
        if (loop >= len) loop = 0;
        end = len;
        string kind = period ? "ciclo" : "entera";
        return kind + " " + score.ToString("0.00", CultureInfo.InvariantCulture);
    }

    /* The last seconds of an extended upload are one more copy of the jingle.
       A full cross-correlation finds that distance, then the shortest lag that
       still matches becomes one seamless cycle. */
    private static bool FindPeriod(float[] st, int n, out int start, out int end, out double score) {
        start = 0;
        end = n;
        score = 0;
        const int Step = 10;
        int low = Rate / Step;
        int m = n / Step;
        if (m < low * 16) return false;
        float[] x = new float[m];
        for (int i = 0; i < m; i++) {
            double s = 0;
            int o = i * Step;
            for (int k = 0; k < Step; k++) s += st[(o + k) * 2] + st[(o + k) * 2 + 1];
            x[i] = (float)(s / (Step * 2));
        }
        int hop = low / 20;
        int frames = m / hop;
        float[] env = new float[frames];
        for (int i = 0; i < frames; i++) {
            double e = 0;
            int o = i * hop;
            for (int k = 0; k < hop && o + k < m; k++) e += x[o + k] * x[o + k];
            env[i] = (float)Math.Sqrt(e / hop);
        }
        float loud = Percentile(env, 0.5f);
        int first = 0, last = frames - 1;
        while (first < frames && env[first] < loud * 0.12f) first++;
        while (last > first && env[last] < loud * 0.35f) last--;
        int active0 = first * hop;
        int active1 = Math.Min(m, (last + 1) * hop);
        int probe = 3 * low;
        int span = active1 - active0;
        if (span < probe + 16 * low) return false;
        int chosenPeriod = 0, chosenAt = 0;
        double chosen = 0;
        for (int t = 1; t <= 3; t++) {
            int at = active0 + (span - probe) * t / 4;
            int period;
            double sc;
            if (!TryAt(x, active0, at, probe, low, out period, out sc)) continue;
            if (sc > chosen) { chosen = sc; chosenPeriod = period; chosenAt = at; }
        }
        score = chosen;
        if (chosenPeriod == 0 || chosen < 0.80) return false;
        int coarse = chosenPeriod * Step;
        int join = Math.Min(n, chosenAt * Step);
        int check = Math.Min(2 * Rate, coarse / 3);
        if (join - coarse - check < 0 || join + 64 > n) return false;
        int fine = 0;
        double fineR = -1;
        for (int d = -Rate / 40; d <= Rate / 40; d += 2) {
            int p = coarse + d;
            double r = Corr(st, join - p - check, join - check, check);
            if (r > fineR) { fineR = r; fine = d; }
        }
        score = fineR;
        if (fineR < 0.85) return false;
        start = join - (coarse + fine);
        end = join;
        if (start < 0 || end - start < 8 * Rate || end > n) return false;
        return true;
    }

    private static bool TryAt(float[] x, int active0, int needleAt, int probe, int low, out int period, out double score) {
        period = 0;
        score = 0;
        int minLag = 8 * low;
        int maxLag = Math.Min(180 * low, needleAt - active0);
        if (maxLag <= minLag || needleAt + probe > x.Length) return false;
        int origin = needleAt - maxLag;
        float[] hay = new float[maxLag + probe];
        Array.Copy(x, origin, hay, 0, hay.Length);
        int needle = maxLag;
        double[] corr = Cross(hay, needle, probe);
        double[] energy = new double[hay.Length + 1];
        for (int i = 0; i < hay.Length; i++) energy[i + 1] = energy[i] + hay[i] * (double)hay[i];
        double nrgN = 0;
        for (int i = 0; i < probe; i++) nrgN += hay[needle + i] * (double)hay[needle + i];
        if (nrgN < 1e-8) return false;
        int bestLag = 0;
        double best = 0, sum = 0;
        int counted = 0;
        for (int lag = minLag; lag <= maxLag; lag++) {
            double r = CosAt(corr, energy, nrgN, needle, probe, lag);
            if (r > best) { best = r; bestLag = lag; }
            sum += r;
            counted++;
        }
        score = best;
        if (bestLag == 0 || best < 0.80) return false;
        if (counted > 0 && best - sum / counted < 0.12) return false;
        period = bestLag;
        for (int lag = minLag; lag <= bestLag; lag++) {
            double r = CosAt(corr, energy, nrgN, needle, probe, lag);
            if (r < best - 0.03 || r < 0.88) continue;
            bool peak = true;
            int rad = low / 20;
            for (int d = -rad; d <= rad && peak; d++) {
                if (d == 0 || lag + d < minLag || lag + d > maxLag) continue;
                if (CosAt(corr, energy, nrgN, needle, probe, lag + d) > r + 0.005) peak = false;
            }
            if (peak) { period = lag; break; }
        }
        return true;
    }

    private static double CosAt(double[] corr, double[] energy, double nrgN, int needle, int probe, int lag) {
        int k = needle - lag;
        if (k < 0) return 0;
        double den = (energy[k + probe] - energy[k]) * nrgN;
        return den > 1e-8 ? corr[k] / Math.Sqrt(den) : 0;
    }

    private static void SelfTest() {
        int period = Rate * 12;
        int n = period * 3;
        float[] cycle = new float[period];
        float[] pcm = new float[n * 2];
        for (int i = 0; i < period; i++)
            cycle[i] = (float)Math.Sin(i * 0.05) * 0.5f + (float)Math.Sin(i * 0.013) * 0.3f;
        for (int i = 0; i < n; i++) pcm[i * 2] = pcm[i * 2 + 1] = cycle[i % period];
        int a, b;
        double sc;
        bool ok = FindPeriod(pcm, n, out a, out b, out sc);
        Console.WriteLine(string.Format(CultureInfo.InvariantCulture, "prueba {0} {1:0.00}s score {2:0.00}", ok ? "ciclo" : "falla", (b - a) / (double)Rate, sc));
    }

    private static double[] Cross(float[] hay, int needle, int probe) {
        int size = 1;
        while (size < hay.Length + probe) size <<= 1;
        double[] reA = new double[size], imA = new double[size];
        double[] reB = new double[size], imB = new double[size];
        for (int i = 0; i < hay.Length; i++) reA[i] = hay[i];
        for (int i = 0; i < probe; i++) reB[i] = hay[needle + i];
        Fft(reA, imA, false);
        Fft(reB, imB, false);
        for (int i = 0; i < size; i++) {
            double r = reA[i] * reB[i] + imA[i] * imB[i];
            double m = imA[i] * reB[i] - reA[i] * imB[i];
            reA[i] = r;
            imA[i] = m;
        }
        Fft(reA, imA, true);
        return reA;
    }

    private static void Fft(double[] re, double[] im, bool invert) {
        int n = re.Length;
        for (int i = 1, j = 0; i < n; i++) {
            int bit = n >> 1;
            for (; (j & bit) != 0; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) {
                double tr = re[i]; re[i] = re[j]; re[j] = tr;
                double ti = im[i]; im[i] = im[j]; im[j] = ti;
            }
        }
        for (int len = 2; len <= n; len <<= 1) {
            double ang = 2 * Math.PI / len * (invert ? -1 : 1);
            double wRe = Math.Cos(ang), wIm = Math.Sin(ang);
            for (int i = 0; i < n; i += len) {
                double cRe = 1, cIm = 0;
                int half = len >> 1;
                for (int j = 0; j < half; j++) {
                    int u = i + j, v = u + half;
                    double vr = re[v] * cRe - im[v] * cIm;
                    double vi = re[v] * cIm + im[v] * cRe;
                    re[v] = re[u] - vr; im[v] = im[u] - vi;
                    re[u] += vr; im[u] += vi;
                    double nRe = cRe * wRe - cIm * wIm;
                    cIm = cRe * wIm + cIm * wRe;
                    cRe = nRe;
                }
            }
        }
        if (invert) {
            for (int i = 0; i < n; i++) { re[i] /= n; im[i] /= n; }
        }
    }

    private static void Trim(float[] st, int n, out int a, out int b) {
        const float Floor = 0.004f;
        a = 0;
        b = n;
        while (a < n - 1 && Math.Abs(st[2 * a]) < Floor && Math.Abs(st[2 * a + 1]) < Floor) a++;
        while (b > a + 1 && Math.Abs(st[2 * (b - 1)]) < Floor && Math.Abs(st[2 * (b - 1) + 1]) < Floor) b--;
        a = Math.Max(0, a - Rate / 50);
        b = Math.Min(n, b + Rate / 50);
    }

    private static void Crossfade(float[] pcm, int n, int fade) {
        fade = Math.Min(fade, n / 4);
        for (int i = 0; i < fade; i++) {
            float w = (i + 1) / (float)fade;
            int d = (n - fade + i) * 2;
            int s = i * 2;
            pcm[d] = pcm[d] * (1 - w) + pcm[s] * w;
            pcm[d + 1] = pcm[d + 1] * (1 - w) + pcm[s + 1] * w;
        }
    }

    private static float Percentile(float[] v, float p) {
        float[] c = (float[])v.Clone();
        Array.Sort(c);
        return c[Math.Min(c.Length - 1, (int)(c.Length * p))];
    }

    private static double Corr(float[] st, int a, int b, int n) {
        if (a < 0 || b < 0 || (a + n) * 2 > st.Length || (b + n) * 2 > st.Length) return -1;
        double xy = 0, xx = 0, yy = 0;
        for (int i = 0; i < n; i++) {
            float u = st[(a + i) * 2] + st[(a + i) * 2 + 1];
            float v = st[(b + i) * 2] + st[(b + i) * 2 + 1];
            xy += u * v; xx += u * u; yy += v * v;
        }
        if (xx < 1e-6 || yy < 1e-6) return 0;
        return xy / Math.Sqrt(xx * yy);
    }

    private static float[] Decode(string file) {
        var psi = new ProcessStartInfo(s_ffmpeg, "-v error -i \"" + file + "\" -vn -ac 2 -ar " + Rate + " -f f32le pipe:1") {
            UseShellExecute = false, RedirectStandardOutput = true, CreateNoWindow = true
        };
        using var p = Process.Start(psi);
        var ms = new MemoryStream();
        p.StandardOutput.BaseStream.CopyTo(ms);
        p.WaitForExit();
        if (p.ExitCode != 0) throw new Exception("ffmpeg no pudo leer " + file);
        byte[] b = ms.ToArray();
        float[] f = new float[b.Length / 4];
        Buffer.BlockCopy(b, 0, f, 0, f.Length * 4);
        return f;
    }

    private static void Encode(float[] pcm, string dest, string codec) {
        var psi = new ProcessStartInfo(s_ffmpeg, "-v error -y -f f32le -ar " + Rate + " -ac 2 -i pipe:0 -map_metadata -1 " + codec + " \"" + dest + "\"") {
            UseShellExecute = false, RedirectStandardInput = true, CreateNoWindow = true
        };
        using var p = Process.Start(psi);
        byte[] b = new byte[pcm.Length * 4];
        Buffer.BlockCopy(pcm, 0, b, 0, b.Length);
        p.StandardInput.BaseStream.Write(b, 0, b.Length);
        p.StandardInput.Close();
        p.WaitForExit();
        if (p.ExitCode != 0) throw new Exception("ffmpeg no pudo escribir " + dest);
    }
}
