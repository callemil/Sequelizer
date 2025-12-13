---
layout: default
title: Examples
---
# Examples

## Consider making a shortcut

See [Getting Started](../getting-started.md) to learn how to build Sequelizer.  First, the build instructions leave you with a `sequelizer` binary in your `build` directory.  You might want to alias it for easier use:
```bash
some_dir $ alias sequelizer='<whatever_your_path_is>/build/sequelizer'
```
Now you can run Sequelizer commands with `sequelizer`.  The examples below assume you have done this.

## A Look at Fast5 Files
You can use Sequelizer to analyze Fast5 files.  A mature file format that holds nanopore information, both raw DNA measurements and lots of useful metadata.  We (and you?) want circuits, computers, programs, networks working on this data to extract relevant genomic inisights anytime, anywhere.

To play around you'll want some Fast5 files on your system.  Here's how to download a sample dataset 
```bash
# Get some Fast5 files (this is ~2.4GB, with a 15MB/s download, it should take ~2.5 min)
some_dir $ wget https://ont-exd-int-s3-euwst1-epi2me-labs.s3-eu-west-1.amazonaws.com/fast5_tutorial/sample_fast5.tar
# Uncompress
some_dir $ tar -xvf sample_fast5.tar
# See what you got
some_dir $ ls
... sample_fast5/ ...
```
Now that you have some data in the `sample_fast5` directory, you can use Sequelizer to analyze the Fast5 files.  Just point it to the directory with the `--recursive` flag to find all Fast5 files within:
```bash
# sequelize it
some_dir $ ./sequelizer fast5 sample_fast5 --recursive
Discovering Fast5 files...
Found 5 files, analyzing...
[[████████████████████████████████████████] 100% (5/5)

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
What happened?  Sequelizer scanned the Fast5 files, extracted some basic statistics about the raw signal data, and printed a summary report.  You can see how many reads were processed in the directory you scanned, their lengths, and other useful information.  Almost 30 bits per sample were used to store the raw signal data?  Awful if true.

If you want to know a little more about each read that was processed you could ask for a summary report to be made.
```bash
some_dir $ ./sequelizer fast5 sample_fast5 --recursive --summary
```
This will create a `sequelizer_summary.txt` file in your current directory with a line for each read processed, and some basic statistics about it.

## Before You Go
Please help Sequelizer development.  Tell us what else you'd like to see here!