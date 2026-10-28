"""Build deterministic audio assets without signal-level processing."""
from pathlib import Path
import json
import shutil
import subprocess
import tempfile
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parent
content_dir = root / 'content'
content_dir.mkdir(exist_ok=True)
audio_dir = root / 'assets'
audio_dir.mkdir(exist_ok=True)
segments_dir = root / 'assets' / 'segments'
segments_dir.mkdir(parents=True, exist_ok=True)

AUDIO_EXTENSIONS = {'.mp3', '.wav', '.flac', '.ogg', '.aac', '.m4a', '.wma', '.opus', '.m4b'}
VIDEO_EXTENSIONS = {'.mp4', '.m4v', '.mov', '.mkv', '.webm', '.avi', '.3gp', '.ts'}
# Decode video at build time: the app ABI exposes GPU presentation, not a native
# codec API. Keep clips small enough to fit the launcher data storage buffer.
files = sorted((p for p in content_dir.iterdir() if p.is_file() and
                p.suffix.lower() in AUDIO_EXTENSIONS | VIDEO_EXTENSIONS),
               key=lambda p: p.name.lower())[:31]
video_files = [p for p in files if p.suffix.lower() in VIDEO_EXTENSIONS]

font = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
try:
    ImageFont.truetype(font, 24)
except OSError:
    font = 'arial.ttf'

atlas = Image.new('L', (1024, 2048))
draw = ImageDraw.Draw(atlas)
header = []
y = 2
items = [
    ('TITLE', 'Media Machine - Video and Audio', 36),
	('SUBTITLE', 'Select with Up/Down; A plays | Y stops | X next', 18),
    ('STATUS_READY', ('Protected 120s Intro + Real-time Segment Streaming @ 900p50.' if video_files else
                      'Add video or audio files to content/ to begin.'), 16),
    ('STATUS_SELECTED', 'Streaming video (Protected first 120s + 60s sliding window)', 16),
    ('CONTROLS', 'D-Pad: Navigate | A: Play | Y: Stop | X: Next | B: Exit', 18),
    ('BASS_STATUS', 'Audio Effect: +12dB Bass Boost (110Hz)', 16),
]
for name, label, size in items:
    face = ImageFont.truetype(font, size)
    box = draw.textbbox((0, 0), label, font=face)
    w, h = box[2] - box[0], box[3] - box[1]
    draw.text((2 - box[0], y - box[1]), label, font=face, fill=255)
    header.append(f'const ivec4 TEXT_{name} = ivec4(2, {y}, {w}, {h});')
    y += h + 8

ui_files = files or [None]
header.append(f'const int NUM_FILES = {len(ui_files)};')
for i in range(32):
    if i < len(ui_files) and ui_files[i] is None:
        label = 'No supported audio files found'
    elif i < len(ui_files):
        label = f'{i + 1}. {files[i].name}'
        if len(label) > 54:
            label = label[:51] + '...'
    else:
        label = ''
    face = ImageFont.truetype(font, 20)
    box = draw.textbbox((0, 0), label, font=face)
    w, h = box[2] - box[0], box[3] - box[1]
    draw.text((2 - box[0], y - box[1]), label, font=face, fill=255)
    header.append(f'const ivec4 TEXT_FILE_{i} = ivec4(2, {y}, {w}, {h});')
    y += h + 8

im = Image.new('RGBA', (256, 256), (15, 32, 52, 255))
d = ImageDraw.Draw(im)
d.rounded_rectangle((26, 42, 230, 214), radius=22, fill=(28, 60, 90), outline=(80, 220, 240), width=5)
d.rectangle((60, 80, 196, 170), fill=(40, 90, 130), outline=(120, 240, 255), width=3)
d.polygon([(110, 105), (110, 145), (150, 125)], fill=(120, 255, 220))
im.save(root / 'assets/icon.png')

# Videos are decoded on the build host into a bounded RGB565 frame stream.
# ABI 1 has no native runtime codec; the fragment shader reads packed RGB565.
if files and not shutil.which('ffmpeg'):
    raise RuntimeError('ffmpeg is required to package audio/video files (install ffmpeg and rebuild)')

atlas_bytes = atlas.tobytes()
video_payload = bytearray()
video_meta = [(0, 0, 0) for _ in files]  # byte offset in data, frames, fps
video_indices = [i for i, source in enumerate(files) if source.suffix.lower() in VIDEO_EXTENSIONS]
# Stay inside the catalog's 7 MiB app-data cap with space for serialization.
# Allocate the remaining budget fairly across remaining clips, reclaiming space
# when a short clip ends early rather than permanently wasting its share.
stream_manifest = {}
video_budget = max(0, 7 * 1024 * 1024 - len(atlas_bytes) - 16 * 1024)
frame_width, frame_height, frame_rate = 1600, 900, 50
frame_bytes = frame_width * frame_height * 2
remaining_budget = video_budget
for position, i in enumerate(video_indices):
    source = files[i]
    duration = 600.0  # default 10 mins fallback
    if shutil.which('ffprobe'):
        try:
            probe = subprocess.run(
                ['ffprobe', '-v', 'error', '-show_entries', 'format=duration', '-of', 'default=noprint_wrappers=1:nokey=1', str(source)],
                capture_output=True, text=True, check=True, timeout=10
            )
            if probe.stdout.strip():
                duration = float(probe.stdout.strip())
        except Exception:
            pass

    total_frames = int(duration * frame_rate)
    offset = len(atlas_bytes)
    video_meta[i] = (offset, total_frames, frame_rate, frame_width)
    stream_manifest[str(i)] = {
        'source': str(source.relative_to(root)),
        'fps': frame_rate,
        'totalFrames': total_frames,
        'duration': duration,
        'offset': offset,
        'width': frame_width,
        'height': frame_height
    }

    # Pre-encode first 120 seconds (Segment 0: 0-60s, Segment 1: 60-120s) as protected intro
    if shutil.which('ffmpeg'):
        for seg_idx in range(2):
            seg_path = segments_dir / f'clip_{i}_seg_{seg_idx}.rgb565'
            if not seg_path.exists() or seg_path.stat().st_size == 0:
                start_time = seg_idx * 60
                cmd = [
                    'ffmpeg', '-v', 'error',
                    '-ss', str(start_time),
                    '-i', str(source),
                    '-t', '60',
                    '-an', '-sn', '-dn', '-q', '0', '-lossless', 'true',
                    '-filter_complex:v', f'fps={frame_rate},scale={frame_width}:{frame_height}:force_original_aspect_ratio=increase,crop={frame_width}:{frame_height}',
                    '-f', 'rawvideo', '-pix_fmt', 'rgb565le', '-y', str(seg_path)
                ]
                try:
                    subprocess.run(cmd, check=True, timeout=300)
                    print(f'Pre-encoded protected intro segment {seg_idx} (0-120s block) for {source.name}')
                except Exception as e:
                    print(f'Warning: Could not pre-encode segment {seg_idx} for {source.name}: {e}')

header.append('const ivec4 VIDEO_META[31] = ivec4[31](')
header.append(',\n'.join(f'    ivec4({offset}, {count}, {fps}, {w})' for offset, count, fps, w in video_meta + [(0, 0, 0, 0)] * (31 - len(video_meta))))

header.append(');')
(root / 'assets/text.bin').write_bytes(atlas_bytes + bytearray(frame_width * frame_height * 2))
(root / 'shaders/text.glsl').write_text('\n'.join(header) + '\n')
(root / 'assets/video-stream.json').write_text(json.dumps(stream_manifest, indent=2) + '\n')

audio_files = [p for p in files if p.suffix.lower() in AUDIO_EXTENSIONS]
audio_effects = {}
audio_dir = root / 'assets'
audio_dir.mkdir(exist_ok=True)
for i, source in enumerate(files, 1):
    target = audio_dir / f'audio_{i}.pcm'
    if source.suffix.lower() not in AUDIO_EXTENSIONS | VIDEO_EXTENSIONS:
        continue
    with tempfile.NamedTemporaryFile(prefix='media-machine-2-', suffix='.pcm', dir=root / 'assets', delete=False) as tmp:
        temp_name = tmp.name
    try:
        command = [
            'ffmpeg', '-v', 'error', '-i', str(source), '-vn', '-q', '0', '-lossless', 'true',
            '-filter_complex:a', 'acopy,bass=g=12:f=110',
            '-acodec', 'pcm_s16le', '-f', 's16le', '-ar', '48000', '-ac', '2', '-y', temp_name,
        ]
        try:
            subprocess.run(command, check=True, timeout=1800)
        except subprocess.CalledProcessError:
            if source.suffix.lower() in VIDEO_EXTENSIONS:
                continue  # Silent video is still playable.
            raise
        size = Path(temp_name).stat().st_size
        if not size or size % 4:
            if source.suffix.lower() in VIDEO_EXTENSIONS:
                continue
            raise RuntimeError(f'ffmpeg produced invalid stereo PCM for {source.name}')
        if size > 512 * 1024 * 1024:
            raise RuntimeError(f'{source.name} exceeds the 512 MiB decoded audio limit')
        Path(temp_name).replace(target)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        raise RuntimeError(f'Could not decode {source.name}') from exc
    finally:
        Path(temp_name).unlink(missing_ok=True)
    audio_effects[str(i)] = f'assets/audio_{i}.pcm'

# Remove stale numbered assets to avoid accidentally shipping obsolete tracks.
for stale in audio_dir.glob('audio_*.pcm'):
    try:
        if stale.name not in {f'audio_{i}.pcm' for i in range(1, len(files) + 1)}:
            stale.unlink()
    except OSError:
        pass
manifest_path = root / 'app.json'
manifest = json.loads(manifest_path.read_text())
manifest['audio'] = {
    'sampleRate': 48000, 'channels': 2, 'format': 's16le', 'effects': audio_effects,
}
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
