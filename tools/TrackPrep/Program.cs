using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

internal sealed class Track {
    public string Stem;
    public string Source;
    public string Title;
    public int LoopStart;
    public int End;
    public int Total;
    public string Method;
    public double Score;
    public double Corr;
}

internal static class Program {
    private const int Rate = 44100;
    private const int Hop = 512;
    private const int Fft = 2048;
    private const int Bands = 40;
    private static string s_ffmpeg;

    private static int Main(string[] args) {
        var opt = new Dictionary<string, string>();
        for (int i = 0; i + 1 < args.Length; i += 2) opt[args[i].TrimStart('-')] = args[i + 1];
        s_ffmpeg = opt.ContainsKey("ffmpeg") ? opt["ffmpeg"] : "ffmpeg";
        if (opt.ContainsKey("music"))
            return ConvertFolder(opt["music"], opt.GetValueOrDefault("out98"), opt.GetValueOrDefault("out2002"));
        string songs = opt["songs"];
        string outDir = opt["out"];
        string only = opt.ContainsKey("only") ? opt["only"] : null;
        List<Track> tracks = ReadManifest(opt["manifest"], songs);
        Directory.CreateDirectory(Path.Combine(outDir, "kof98"));
        Directory.CreateDirectory(Path.Combine(outDir, "kof2002"));
        var todo = tracks.Where(t => only == null || t.Stem == only).ToList();
        int workers = Math.Clamp(Environment.ProcessorCount / 2, 2, 6);
        int done = 0;
        Parallel.ForEach(todo, new ParallelOptions { MaxDegreeOfParallelism = workers }, t => {
            Process(t, outDir);
            int n = System.Threading.Interlocked.Increment(ref done);
            Console.WriteLine($"[{n}/{todo.Count}] {t.Stem,-14} {t.Method,-6} loop {t.LoopStart / (double)Rate,7:0.000}s fin {t.End / (double)Rate,7:0.000}s de {t.Total / (double)Rate,7:0.000}s score {t.Score:0.000} corr {t.Corr:0.000}");
        });
        WriteReport(Path.Combine(outDir, "loops.csv"), tracks.Where(t => t.Method != null));
        if (opt.ContainsKey("header") && only == null) WriteHeader(opt["header"], tracks);
        return 0;
    }

    private static List<Track> ReadManifest(string path, string songs) {
        var dirs = new Dictionary<string, string>();
        var list = new List<Track>();
        foreach (string raw in File.ReadAllLines(path, Encoding.UTF8)) {
            string line = raw.Trim();
            if (line.Length == 0 || line[0] == '#') continue;
            if (line[0] == '@') {
                int eq = line.IndexOf('=');
                dirs[line.Substring(1, eq - 1)] = Path.GetFullPath(Path.Combine(songs, line.Substring(eq + 1)));
                continue;
            }
            string[] p = raw.Split('|');
            string dir = dirs[p[1]];
            var hits = Directory.GetFiles(dir).Where(f => Path.GetFileName(f).StartsWith(p[2], StringComparison.OrdinalIgnoreCase)).ToList();
            if (hits.Count != 1) throw new Exception($"{p[0]}: {hits.Count} archivos empiezan con '{p[2]}' en {dir}");
            list.Add(new Track { Stem = p[0], Source = hits[0], Title = p[3].Trim() });
        }
        return list;
    }

    private static readonly string[] AudioExt = { ".mp3", ".flac", ".wav", ".ogg", ".m4a", ".aac", ".wma", ".opus", ".aiff", ".aif", ".mp4" };

    /* Music folder mode: every audio file under `src` (any depth) becomes an
       album song for the music mods, keeping the folder structure:
       <out98>\<album>\<song>.ogg (44.1 kHz) and <out2002>\<album>\<song>.ogg
       (48 kHz), each with <song>.loop holding the loop start in seconds.
       Songs already converted (destination newer than the source) are skipped. */
    private static int ConvertFolder(string src, string out98, string out2002) {
        if (out98 == null && out2002 == null) {
            Console.WriteLine("Indica --out98 y/o --out2002 (la carpeta Music de cada juego).");
            return 1;
        }
        src = Path.GetFullPath(src);
        var files = Directory.EnumerateFiles(src, "*", SearchOption.AllDirectories)
            .Where(f => AudioExt.Contains(Path.GetExtension(f).ToLowerInvariant()))
            .Where(f => out98 == null || !Path.GetFullPath(f).StartsWith(Path.GetFullPath(out98), StringComparison.OrdinalIgnoreCase))
            .Where(f => out2002 == null || !Path.GetFullPath(f).StartsWith(Path.GetFullPath(out2002), StringComparison.OrdinalIgnoreCase))
            .OrderBy(f => f, StringComparer.OrdinalIgnoreCase).ToList();
        int done = 0, skipped = 0, failed = 0;
        int workers = Math.Clamp(Environment.ProcessorCount / 2, 2, 6);
        Parallel.ForEach(files, new ParallelOptions { MaxDegreeOfParallelism = workers }, f => {
            string rel = Path.ChangeExtension(Path.GetRelativePath(src, f), null);
            string d98 = out98 == null ? null : Path.Combine(out98, rel + ".ogg");
            string d2002 = out2002 == null ? null : Path.Combine(out2002, rel + ".ogg");
            if (UpToDate(f, d98) && UpToDate(f, d2002)) {
                System.Threading.Interlocked.Increment(ref skipped);
                return;
            }
            var t = new Track { Stem = rel, Source = f, Title = Path.GetFileNameWithoutExtension(f) };
            try {
                Render(t, d98, d2002, true);
                int n = System.Threading.Interlocked.Increment(ref done);
                Console.WriteLine($"[{n}] {rel}  ({t.Method}, loop {t.LoopStart / (double)Rate:0.000}s)");
            } catch (Exception e) {
                System.Threading.Interlocked.Increment(ref failed);
                Console.WriteLine($"ERROR {rel}: {e.Message}");
            }
        });
        Console.WriteLine($"Convertidas: {done}  ya estaban: {skipped}  con error: {failed}");
        return failed > 0 ? 2 : 0;
    }

    private static bool UpToDate(string source, string dest) {
        if (dest == null) return true;
        return File.Exists(dest) && File.GetLastWriteTimeUtc(dest) >= File.GetLastWriteTimeUtc(source);
    }

    private static void Process(Track t, string outDir) {
        Render(t, Path.Combine(outDir, "kof98", t.Stem + ".ogg"), Path.Combine(outDir, "kof2002", t.Stem + ".ogg"), false);
    }

    private static void Render(Track t, string dest98, string dest2002, bool loop98) {
        float[] st = Decode(t.Source);
        int total = st.Length / 2;
        float[] mono = new float[total];
        for (int i = 0; i < total; i++) mono[i] = 0.5f * (st[2 * i] + st[2 * i + 1]);
        t.Total = total;
        if (!FindLoop(mono, t)) FullLoop(mono, t);
        float[] pcm = new float[t.End * 2];
        Array.Copy(st, pcm, pcm.Length);
        if (t.Method == "repite") Crossfade(pcm, t.LoopStart, t.End, Rate / 100);
        else FadeTail(pcm, t.End, Rate / 200);
        string loop = (t.LoopStart / (double)Rate).ToString("0.000000", CultureInfo.InvariantCulture);
        if (dest98 != null) {
            Directory.CreateDirectory(Path.GetDirectoryName(dest98));
            Encode(pcm, dest98, "-c:a libvorbis -q:a 5");
            if (loop98) File.WriteAllText(Path.ChangeExtension(dest98, ".loop"), loop);
        }
        if (dest2002 != null) {
            Directory.CreateDirectory(Path.GetDirectoryName(dest2002));
            Encode(pcm, dest2002, "-ar 48000 -c:a libvorbis -q:a 4");
            File.WriteAllText(Path.ChangeExtension(dest2002, ".loop"), loop);
        }
    }

    private static float[] Decode(string file) {
        var psi = new ProcessStartInfo(s_ffmpeg, $"-v error -i \"{file}\" -vn -ac 2 -ar {Rate} -f f32le pipe:1") {
            UseShellExecute = false, RedirectStandardOutput = true, CreateNoWindow = true
        };
        using var p = System.Diagnostics.Process.Start(psi);
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
        var psi = new ProcessStartInfo(s_ffmpeg, $"-v error -y -f f32le -ar {Rate} -ac 2 -i pipe:0 -map_metadata -1 {codec} \"{dest}\"") {
            UseShellExecute = false, RedirectStandardInput = true, CreateNoWindow = true
        };
        using var p = System.Diagnostics.Process.Start(psi);
        byte[] b = new byte[pcm.Length * 4];
        Buffer.BlockCopy(pcm, 0, b, 0, b.Length);
        p.StandardInput.BaseStream.Write(b, 0, b.Length);
        p.StandardInput.Close();
        p.WaitForExit();
        if (p.ExitCode != 0) throw new Exception("ffmpeg no pudo escribir " + dest);
    }

    private static bool FindLoop(float[] mono, Track t) {
        int n4 = mono.Length / 4;
        float[] low = new float[n4];
        for (int i = 0; i < n4; i++) low[i] = 0.25f * (mono[4 * i] + mono[4 * i + 1] + mono[4 * i + 2] + mono[4 * i + 3]);
        int frames = (n4 - Fft) / Hop;
        if (frames < 200) return false;
        float[][] feat = Features(low, frames, out float[] rms);
        float[] sorted = (float[])rms.Clone();
        Array.Sort(sorted);
        float loud = sorted[(int)(sorted.Length * 0.75)];
        int first = 0, last = frames - 1;
        while (first < frames && rms[first] < loud * 0.1f) first++;
        while (last > first && rms[last] < loud * 0.3f) last--;
        double fps = Rate / 4.0 / Hop;
        int win = (int)(6 * fps);
        int minLag = (int)(25 * fps);
        const double Threshold = 0.95;
        int attempts = 0;
        for (int lag = last - first - win; lag >= minLag && attempts < 60; lag--) {
            int tFrom = first, tTo = last - lag - win;
            if (tTo <= tFrom) continue;
            double sum = 0;
            for (int k = 0; k < win; k++) sum += Dot(feat[tFrom + k], feat[tFrom + lag + k]);
            int bestT = -1;
            double bestMean = 0;
            for (int s = tFrom; ; s++) {
                double mean = sum / win;
                if (mean >= Threshold) {
                    bestMean = mean;
                    bestT = s;
                }
                if (s + 1 > tTo) break;
                sum += Dot(feat[s + win], feat[s + lag + win]) - Dot(feat[s], feat[s + lag]);
            }
            if (bestT < 0 || bestT + lag + win < last - (int)(10 * fps)) continue;
            attempts++;
            int a = bestT * Hop * 4 + Rate / 4;
            int coarse = lag * Hop * 4;
            int len = 3 * Rate;
            if (a + coarse + 2 * len + 2048 >= mono.Length) continue;
            int best = 0;
            double bestR = -1;
            for (int d = -1024; d <= 1024; d++) {
                double r = Corr(mono, a, a + coarse + d, len);
                if (r > bestR) { bestR = r; best = d; }
            }
            double r2 = Corr(mono, a + len, a + len + coarse + best, len);
            if (bestR < 0.8 || r2 < 0.75) continue;
            t.LoopStart = a;
            t.End = a + coarse + best;
            t.Method = "repite";
            t.Score = bestMean;
            t.Corr = Math.Min(bestR, r2);
            return true;
        }
        return false;
    }

    private static void FullLoop(float[] mono, Track t) {
        const float Floor = 0.003f;
        int s = 0, e = mono.Length - 1;
        while (s < mono.Length && Math.Abs(mono[s]) < Floor) s++;
        while (e > s && Math.Abs(mono[e]) < Floor) e--;
        t.LoopStart = Math.Max(0, s - Rate / 100);
        t.End = Math.Min(mono.Length, e + Rate / 10);
        t.Method = "entera";
        t.Score = 0;
        t.Corr = 0;
    }

    private static void Crossfade(float[] pcm, int loop, int end, int len) {
        len = Math.Min(len, loop);
        for (int i = 0; i < len; i++) {
            float w = (i + 1) / (float)len;
            int dst = (end - len + i) * 2;
            int src = (loop - len + i) * 2;
            for (int c = 0; c < 2; c++) pcm[dst + c] = pcm[dst + c] * (1 - w) + pcm[src + c] * w;
        }
    }

    private static void FadeTail(float[] pcm, int end, int len) {
        for (int i = 0; i < len && i < end; i++) {
            float w = i / (float)len;
            int at = (end - 1 - i) * 2;
            pcm[at] *= w;
            pcm[at + 1] *= w;
        }
    }

    private static double Corr(float[] x, int a, int b, int n) {
        double xy = 0, xx = 0, yy = 0;
        for (int i = 0; i < n; i++) {
            float u = x[a + i], v = x[b + i];
            xy += u * v; xx += u * u; yy += v * v;
        }
        if (xx < 1e-6 || yy < 1e-6) return 0;
        return xy / Math.Sqrt(xx * yy);
    }

    private static double Dot(float[] a, float[] b) {
        double s = 0;
        for (int i = 0; i < a.Length; i++) s += a[i] * b[i];
        return s;
    }

    private static float[][] Features(float[] x, int frames, out float[] rms) {
        double binHz = Rate / 4.0 / Fft;
        int[] edge = new int[Bands + 1];
        for (int i = 0; i <= Bands; i++) edge[i] = (int)Math.Round(80 * Math.Pow(5000.0 / 80, i / (double)Bands) / binHz);
        for (int i = 1; i <= Bands; i++) if (edge[i] <= edge[i - 1]) edge[i] = edge[i - 1] + 1;
        float[] window = new float[Fft];
        for (int i = 0; i < Fft; i++) window[i] = (float)(0.5 - 0.5 * Math.Cos(2 * Math.PI * i / Fft));
        var feat = new float[frames][];
        var level = new float[frames];
        Parallel.For(0, frames, f => {
            double[] re = new double[Fft], im = new double[Fft];
            double e = 0;
            for (int i = 0; i < Fft; i++) {
                float v = x[f * Hop + i];
                e += v * v;
                re[i] = v * window[i];
            }
            level[f] = (float)Math.Sqrt(e / Fft);
            FftInPlace(re, im);
            float[] band = new float[Bands];
            double mean = 0;
            for (int b = 0; b < Bands; b++) {
                double s = 0;
                for (int k = edge[b]; k < edge[b + 1]; k++) s += re[k] * re[k] + im[k] * im[k];
                band[b] = (float)Math.Log10(s + 1e-9);
                mean += band[b];
            }
            mean /= Bands;
            double norm = 0;
            for (int b = 0; b < Bands; b++) { band[b] -= (float)mean; norm += band[b] * band[b]; }
            norm = Math.Sqrt(norm);
            for (int b = 0; b < Bands; b++) band[b] = level[f] < 1e-4 || norm < 1e-6 ? 0 : (float)(band[b] / norm);
            feat[f] = band;
        });
        rms = level;
        return feat;
    }

    private static void FftInPlace(double[] re, double[] im) {
        int n = re.Length;
        for (int i = 1, j = 0; i < n; i++) {
            int bit = n >> 1;
            for (; (j & bit) != 0; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) { (re[i], re[j]) = (re[j], re[i]); (im[i], im[j]) = (im[j], im[i]); }
        }
        for (int len = 2; len <= n; len <<= 1) {
            double ang = -2 * Math.PI / len;
            double wr = Math.Cos(ang), wi = Math.Sin(ang);
            for (int i = 0; i < n; i += len) {
                double cr = 1, ci = 0;
                for (int k = 0; k < len / 2; k++) {
                    int u = i + k, v = i + k + len / 2;
                    double tr = re[v] * cr - im[v] * ci, ti = re[v] * ci + im[v] * cr;
                    re[v] = re[u] - tr; im[v] = im[u] - ti;
                    re[u] += tr; im[u] += ti;
                    double nr = cr * wr - ci * wi;
                    ci = cr * wi + ci * wr;
                    cr = nr;
                }
            }
        }
    }

    private static void WriteReport(string path, IEnumerable<Track> tracks) {
        var sb = new StringBuilder("stem,metodo,loop_s,fin_s,original_s,score,corr,titulo\n");
        foreach (var t in tracks.OrderBy(t => t.Stem))
            sb.AppendLine(string.Join(",", t.Stem, t.Method,
                (t.LoopStart / (double)Rate).ToString("0.000", CultureInfo.InvariantCulture),
                (t.End / (double)Rate).ToString("0.000", CultureInfo.InvariantCulture),
                (t.Total / (double)Rate).ToString("0.000", CultureInfo.InvariantCulture),
                t.Score.ToString("0.000", CultureInfo.InvariantCulture),
                t.Corr.ToString("0.000", CultureInfo.InvariantCulture),
                "\"" + t.Title + "\""));
        File.WriteAllText(path, sb.ToString(), new UTF8Encoding(false));
    }

    private static void WriteHeader(string path, List<Track> tracks) {
        var sb = new StringBuilder();
        sb.Append("#pragma once\n\n");
        sb.Append("typedef struct ExtraTrack { const char* stem; const char* title; unsigned loop_bits; } ExtraTrack;\n\n");
        sb.Append("static const ExtraTrack k_extra[] = {\n");
        foreach (var t in tracks) {
            float sec = (float)(t.LoopStart / (double)Rate);
            if (sec < 0.001f) sec = 0.001f;
            uint bits = BitConverter.SingleToUInt32Bits(sec);
            sb.Append($"    {{ \"{t.Stem}\", \"{CString(t.Title)}\", 0x{bits:X8}u }},\n");
        }
        sb.Append("};\n");
        File.WriteAllText(path, sb.ToString(), new UTF8Encoding(false));
    }

    private static string CString(string s) {
        var sb = new StringBuilder();
        foreach (byte b in Encoding.UTF8.GetBytes(s)) {
            if (b == '"' || b == '\\') sb.Append('\\').Append((char)b);
            else if (b < 0x20 || b >= 0x80) sb.Append('\\').Append(Convert.ToString(b, 8).PadLeft(3, '0'));
            else sb.Append((char)b);
        }
        return sb.ToString();
    }
}
