using System;
using System.IO;
using System.Threading;
using NAudio.Vorbis;
using NAudio.Wave;
using OggVorbisEncoder;

internal static class Program {
    private static int Main(string[] args) {
        try {
            if (args.Length >= 2 && args[0] == "silence") {
                File.WriteAllBytes(args[1], BuildSilence());
                return 0;
            }
            if (args.Length >= 3 && args[0] == "battle") {
                int loopMs;
                if (!int.TryParse(args[2], out loopMs)) loopMs = -1;
                PlayBattle(args[1], loopMs);
                return 0;
            }
            if (args.Length >= 1 && args[0].Length > 0) {
                PlayMiddle(args[0]);
                return 0;
            }
            return 1;
        } catch {
            return 1;
        }
    }

    private static void PlayBattle(string path, int loopMs) {
        int rate;
        int channels;
        short[] pcm = Decode(path, out rate, out channels);
        int frames = pcm.Length / channels;
        if (frames < rate / 10) return;
        int loopFrame = 0;
        if (loopMs >= 0) {
            loopFrame = (int)Math.Round(loopMs / 1000.0 * rate);
            if (loopFrame < 0 || loopFrame >= frames - rate / 10) loopFrame = 0;
        }
        SmoothJoin(pcm, channels, frames, loopFrame, rate);
        LoopPcm loop = new LoopPcm(pcm, channels, frames, loopFrame, new WaveFormat(rate, 16, channels));
        using (WaveOutEvent output = new WaveOutEvent()) {
            output.Init(loop);
            output.Volume = 1f;
            output.Play();
            while (output.PlaybackState == PlaybackState.Playing)
                Thread.Sleep(200);
        }
    }

    private static void PlayMiddle(string path) {
        using (VorbisWaveReader reader = new VorbisWaveReader(path)) {
            if (reader.WaveFormat.SampleRate <= 0 || reader.WaveFormat.Channels <= 0) return;
            using (WaveOutEvent output = new WaveOutEvent()) {
                output.Init(new LoopFile(reader));
                output.Volume = 1f;
                output.Play();
                while (output.PlaybackState == PlaybackState.Playing)
                    Thread.Sleep(200);
            }
        }
    }

    private sealed class LoopFile : IWaveProvider {
        private readonly VorbisWaveReader reader;
        public LoopFile(VorbisWaveReader reader) { this.reader = reader; }
        public WaveFormat WaveFormat { get { return reader.WaveFormat; } }
        public int Read(byte[] buffer, int offset, int count) {
            int got = 0;
            while (got < count) {
                int n = reader.Read(buffer, offset + got, count - got);
                if (n > 0) { got += n; continue; }
                reader.CurrentTime = TimeSpan.Zero;
                n = reader.Read(buffer, offset + got, count - got);
                if (n <= 0) break;
                got += n;
            }
            return got;
        }
    }

    private static short[] Decode(string path, out int rate, out int channels) {
        using (VorbisWaveReader reader = new VorbisWaveReader(path)) {
            rate = reader.WaveFormat.SampleRate;
            channels = reader.WaveFormat.Channels;
            MemoryStream raw = new MemoryStream();
            byte[] buf = new byte[16384];
            int n;
            while ((n = reader.Read(buf, 0, buf.Length)) > 0) raw.Write(buf, 0, n);
            return ToPcm(raw.ToArray(), (int)(raw.Length / 4));
        }
    }

    private static int ReadFull(VorbisWaveReader reader, byte[] dest) {
        int got = 0;
        while (got < dest.Length) {
            int n = reader.Read(dest, got, dest.Length - got);
            if (n <= 0) break;
            got += n;
        }
        return got;
    }

    private static short[] ToPcm(byte[] floats, int samples) {
        short[] pcm = new short[samples];
        int count = Math.Min(samples, floats.Length / 4);
        for (int i = 0; i < count; ++i) {
            float s = BitConverter.ToSingle(floats, i * 4);
            if (s > 1f) s = 1f;
            if (s < -1f) s = -1f;
            pcm[i] = (short)(s * 32767f);
        }
        return pcm;
    }

    private static void SmoothJoin(short[] pcm, int channels, int frames, int loopFrame, int rate) {
        int fade = rate / 80;
        if (fade < 8) fade = 8;
        if (loopFrame < 0 || frames - fade <= loopFrame + fade) return;
        for (int i = 0; i < fade; ++i) {
            float t = (i + 1) / (float)fade;
            int fromEnd = (frames - fade + i) * channels;
            int fromLoop = (loopFrame + i) * channels;
            for (int c = 0; c < channels; ++c) {
                float mixed = pcm[fromEnd + c] * (1f - t) + pcm[fromLoop + c] * t;
                if (mixed > 32767f) mixed = 32767f;
                if (mixed < -32768f) mixed = -32768f;
                pcm[fromEnd + c] = (short)mixed;
            }
        }
    }

    private static byte[] BuildSilence() {
        const int Rate = 44100;
        const int Channels = 2;
        const int Samples = Rate;
        float[][] data = new float[Channels][];
        data[0] = new float[Samples];
        data[1] = new float[Samples];
        MemoryStream output = new MemoryStream();
        VorbisInfo info = VorbisInfo.InitVariableBitRate(Channels, Rate, 0.1f);
        OggStream ogg = new OggStream(1);
        Comments comments = new Comments();
        ogg.PacketIn(HeaderPacketBuilder.BuildInfoPacket(info));
        ogg.PacketIn(HeaderPacketBuilder.BuildCommentsPacket(comments));
        ogg.PacketIn(HeaderPacketBuilder.BuildBooksPacket(info));
        FlushPages(ogg, output, true);
        ProcessingState state = ProcessingState.Create(info);
        const int Chunk = 1024;
        for (int read = 0; read <= Samples; read += Chunk) {
            if (read == Samples) state.WriteEndOfStream();
            else state.WriteData(data, Math.Min(Chunk, Samples - read), read);
            OggPacket packet;
            while (!ogg.Finished && state.PacketOut(out packet)) {
                ogg.PacketIn(packet);
                FlushPages(ogg, output, false);
            }
        }
        FlushPages(ogg, output, true);
        return output.ToArray();
    }

    private static void FlushPages(OggStream ogg, Stream output, bool force) {
        OggPage page;
        while (ogg.PageOut(out page, force)) {
            output.Write(page.Header, 0, page.Header.Length);
            output.Write(page.Body, 0, page.Body.Length);
        }
    }

    private sealed class LoopPcm : IWaveProvider {
        private readonly short[] pcm;
        private readonly int channels;
        private readonly int frames;
        private readonly int loopFrame;
        private readonly WaveFormat format;
        private int frame;

        public LoopPcm(short[] pcm, int channels, int frames, int loopFrame, WaveFormat format) {
            this.pcm = pcm;
            this.channels = channels;
            this.frames = frames;
            this.loopFrame = loopFrame;
            this.format = format;
        }

        public WaveFormat WaveFormat { get { return format; } }

        public int Read(byte[] buffer, int offset, int count) {
            int framesOut = count / (2 * channels);
            int wrote = 0;
            for (int i = 0; i < framesOut; ++i) {
                if (frame >= frames) frame = loopFrame;
                int src = frame * channels;
                for (int c = 0; c < channels; ++c) {
                    short v = pcm[src + c];
                    buffer[offset + wrote++] = (byte)(v & 0xFF);
                    buffer[offset + wrote++] = (byte)((v >> 8) & 0xFF);
                }
                frame++;
            }
            return wrote;
        }
    }
}
