---
layout: default
title: Examples
---
# Examples

See [Getting Started](../getting-started.md) to learn how to build Sequelizer.

# A Look at Fast5 Files
You can use Sequelizer to analyze Fast5 files. Here's how to download a sample dataset and run the analysis:
```bash
# Get some Fast5 files (this is ~2.4GB, with a 15MB/s download, it should take ~2.5 min)
some_dir $ wget https://ont-exd-int-s3-euwst1-epi2me-labs.s3-eu-west-1.amazonaws.com/fast5_tutorial/sample_fast5.tar
# Uncompress
some_dir $ tar -xvf sample_fast5.tar
# See what you got
some_dir $ ls
... sample_fast5/ ...
# sequelize it
some_dir $ <some_path>/Sequelizer/build/sequelizer fast5 sample_fast5 --recursive
[████████████████████████████████████████] 100% (5/5)

Sequelizer Fast5 Dataset Analysis Summary
=========================================
Files processed: 5/5 successful
Total file size: 2440.0 MB
Total reads: 20000
Signal statistics:
  Total samples: 679508454
  Average length: 33975 samples
  Range: 1978 - 1199785 samples
  Average bits per sample: 28.73
  Total duration: 2831.3 minutes
  Avg duration: 8.5 seconds
Processing time: 43.02 seconds
```
Please help Sequelizer development.  Tell us what else you'd like to see here!