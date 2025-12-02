---
layout: default
title: Architecture
---
# Architecture & Design Philosophy

## Why Sequelizer Exists

Built in portable C for custom processors and resource-constrained hardware. 
Generate synthetic nanopore streams. Test real-time analysis on actual edge 
devices. Build the infrastructure for genomics everywhere - not just in 
well-funded labs with server racks.

## Design Principles

- **Portable C foundation** - Target RISC-V, custom accelerators, embedded systems
- **Hardware-agnostic core** - Adapts from no SIMD to GPU acceleration
- **Streaming-first** - Designed for real-time data processing, not batch jobs
- **Simulation-driven** - Test pipelines with synthetic data before hardware deployment
