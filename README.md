# **Chronyx**

![platform](https://img.shields.io/badge/platform-ESP8266--12F-red)
![clock](https://img.shields.io/badge/clock-160MHz-critical)
![noise](https://img.shields.io/badge/source-ADC%20%2B%20Clock%20Jitter-orange)
![hash](https://img.shields.io/badge/hash-SHA256-blue)
![network](https://img.shields.io/badge/transport-UDP--Binary-green)
![runtime](https://img.shields.io/badge/runtime-Real--Time-brightgreen)

**Hardware-rooted entropy extraction and cryptographic material generation for ESP8266.**

---

## Overview

**Chronyx** is a firmware-level entropy engine designed to extract **true physical randomness** from the ESP8266 microcontroller.

Instead of relying on pseudo-random generators, Chronyx samples **analog noise**, **clock jitter**, and **instruction-level timing instability** directly from the silicon, converting these physical phenomena into cryptographic-grade entropy using a deterministic hashing pipeline.

Chronyx operates as a **real-time entropy appliance**, continuously mining, condensing, and serving high-quality randomness over a lightweight binary network interface.

---

## Entropy Model

The entropy source is based on the mathematical principle that physical noise and digital jitter are non-deterministic.

Each entropy sample is computed as:

[
E_i = ADC_i \oplus J_i
]

Where:

* (ADC_i) is the raw analog noise read from the ESP8266 ADC
* (J_i) is the instantaneous CPU cycle counter

These values are accumulated into a memory pool and condensed into a fixed-size entropy product:

[
Seed = SHA256(E_1 \parallel E_2 \parallel \dots \parallel E_n)
]

This transforms chaotic physical signals into uniform, cryptographically usable output.

---

## Runtime Architecture

Chronyx runs as a **continuous entropy pipeline**:

```text
ANALOG NOISE        DIGITAL JITTER           ENTROPY CORE             NETWORK
┌─────────────┐   ┌───────────────┐      ┌────────────────┐    ┌─────────────┐
│ ADC Thermal │──►│ CPU Cycle Ji  │──┐   │ 512B Entropy   │──► │ UDP Binary  │
│ Noise       │   │ Instruction   │──┼──►│ Pool + SHA256  │    │ Interface   │
└─────────────┘   │ Timing        │──┘   │ Condenser      │    │             │
                  └───────────────┘      └────────────────┘    └─────────────┘
```

The device never exposes raw noise.
Only hashed, condensed entropy leaves the system.

---

## Deterministic Core, Non-Deterministic Input

Chronyx enforces:

* Fixed 160 MHz CPU frequency
* Disabled WiFi sleep modes
* Continuous entropy harvesting

This ensures:

* Maximum timing jitter
* No power-management smoothing
* Stable entropy flow

All unpredictability comes from physics, not software.

---

## Network Interface

Chronyx exposes a **raw binary UDP interface**.

When queried:

* If entropy is ready, the device returns a fixed-size entropy product
* If not, it continues mining until ready

This design guarantees:

* Zero framing overhead
* Minimal latency
* Deterministic response size

---

## Why Chronyx Exists

Cryptography, distributed systems, secure protocols and time-based systems all depend on **unpredictability**.

Most systems simulate randomness.

Chronyx measures it.

By extracting entropy directly from the microcontroller’s physical behavior, Chronyx provides a **hardware-rooted source of cryptographic material**.

---

## License

**GPLv2**

---

## Philosophy

> “Randomness is not computed.
> It is observed.”

Chronyx is built on the idea that **silicon is never perfectly predictable** — and that unpredictability is a resource.

---

**Author:** David ([@DavidDevGt](https://github.com/DavidDevGt))
