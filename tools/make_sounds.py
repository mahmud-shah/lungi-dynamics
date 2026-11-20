import numpy as np, wave, sys
from scipy.signal import butter, sosfilt
SR = 44100
rng = np.random.default_rng(7)
out = sys.argv[1]

def save(name, x, peak=0.7):
    x = x / (np.max(np.abs(x)) + 1e-9) * peak
    data = (np.clip(x, -1, 1) * 32767).astype(np.int16)
    with wave.open(f"{out}/{name}.wav", "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes(data.tobytes())

def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)

def low(x, hi, order=2):
    return sosfilt(butter(order, hi, btype="low", fs=SR, output="sos"), x)

def seamless(x, fade_s=0.5):
    n = int(fade_s * SR); body = x[:-n].copy(); tail = x[-n:]
    t = np.linspace(0, 1, n); body[:n] = body[:n] * t + tail * (1 - t)
    return body

def env(n, attack, release):
    e = np.ones(n); a = int(attack * SR); r = int(release * SR)
    e[:a] = np.linspace(0, 1, a); e[-r:] *= np.linspace(1, 0, r) ** 2
    return e

# Wind: pink-ish noise through a slowly wandering band-pass, with gusty swells.
n = int(8.5 * SR); t = np.arange(n) / SR
noise = rng.standard_normal(n)
a = band(noise, 150, 700) * (0.6 + 0.4 * np.sin(2 * np.pi * 0.21 * t) * np.sin(2 * np.pi * 0.13 * t + 1))
b = band(noise[::-1].copy(), 500, 1600) * 0.35 * (0.5 + 0.5 * np.sin(2 * np.pi * 0.31 * t + 2))
save("wind_loop", seamless(a + b), 0.6)

# Rain: bright hiss plus random droplet ticks.
n = int(6.5 * SR)
hiss = band(rng.standard_normal(n), 1800, 9000) * 0.5
ticks = np.zeros(n); idx = rng.integers(0, n - 400, 2600)
for i in idx:
    ticks[i:i + 300] += rng.uniform(0.2, 1.0) * np.exp(-np.arange(300) / 40) * rng.standard_normal(300)
save("rain_loop", seamless(hiss + band(ticks, 1500, 7000) * 0.7), 0.5)

# Thunder: a sharp crack followed by a long low rumble with a few rolls.
n = int(4.5 * SR); t = np.arange(n) / SR
crack = band(rng.standard_normal(n), 800, 5000) * np.exp(-t / 0.08)
brown = np.cumsum(rng.standard_normal(n)); brown -= low(brown, 2)
rumble = low(brown, 140, 4) * (np.exp(-t / 1.6) * (1 + 0.6 * np.sin(2 * np.pi * 1.3 * t) ** 2))
save("thunder", crack * 0.6 + rumble / (np.max(np.abs(rumble)) + 1e-9), 0.85)

# Gust: three continuous noise bands cross-faded so the whoosh rises and falls.
n = int(1.4 * SR); t = np.arange(n) / SR
src = rng.standard_normal(n)
lowB, midB, highB = band(src, 200, 500), band(src, 500, 1200), band(src, 1200, 2600)
sweep = np.sin(np.pi * t / t[-1])
w = lowB * (1 - sweep) + midB * sweep + highB * sweep ** 3 * 0.6
save("gust", w * env(n, 0.35, 0.7), 0.6)

# Knot pop: a cork-like pop then a cartoon "boing" (pitch drop with vibrato).
n = int(0.9 * SR); t = np.arange(n) / SR
pop = band(rng.standard_normal(n), 600, 3000) * np.exp(-t / 0.012)
f = 520 * np.exp(-t * 1.6) + 90; phase = 2 * np.pi * np.cumsum(f * (1 + 0.05 * np.sin(2 * np.pi * 14 * t))) / SR
boing = np.sin(phase) * np.exp(-t / 0.35) * (t > 0.05)
save("knot_pop", pop * 1.2 + boing * 0.8, 0.8)

# Fly-up: a rising slide whistle.
n = int(1.1 * SR); t = np.arange(n) / SR
f = 380 * (1 + 2.6 * (t / t[-1]) ** 1.3); phase = 2 * np.pi * np.cumsum(f * (1 + 0.012 * np.sin(2 * np.pi * 6 * t))) / SR
save("fly_up", (np.sin(phase) + 0.25 * np.sin(2 * phase)) * env(n, 0.05, 0.25), 0.6)

# Slip: a quick falling "zip".
n = int(0.32 * SR); t = np.arange(n) / SR
f = 1400 * np.exp(-t * 7) + 180; phase = 2 * np.pi * np.cumsum(f) / SR
save("slip", np.sign(np.sin(phase)) * 0.3 * env(n, 0.005, 0.12) + band(rng.standard_normal(n), 2000, 6000) * 0.2 * np.exp(-t / 0.05), 0.5)
print("ok")
