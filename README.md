# Vice Chess Engine
This is the repo for the Vice chess engine series on YouTube.

You can find the playlist here: [Link to playlist](https://www.youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg)

**From part 97 onwards, this repo has a branch with the code for that video. For exmaple, part 97 code is on branch Chapter 97.**

## Getting in touch / Contributing

You can find the Discord server here: https://discord.gg/9hCUD4n7R2

For now, please do not open pull requests on the repository, it's an archive for the video series. Get in touch via the Discord.

## What is Vice?

Vice is a **V**ideo **I**nstructional **C**hess **E**ngine, written in C.

It was (is) written as part of the follow along series you YouTube by Bluefever Software.

Vice is deliberately simplified - the code structure is by no means to be considered best practice, rather a quick start into understanding the world of computer chesss engine programming.

## Where can I get the binaries and what is the latest version?

You can find the download to the latest release (Vice 1.1) here:

https://bluefeversoft.com/

## I want to copy it, can I?

You can do whatever you want with the code. A lot of engines have been inspired by Vice - not all give credit. Probably it's better to give credit than not.

## What are the main features?

Vice is simple, with the following features:

- Alpha beta search
- Iterative deepening
- Quiescence search
- Transposition table
  - Always replace
- Polyglot opening books
- MVV/LVA move ordering
- Basic evaluation

## bugs to fix:
- The PickMove function needs a BestScore of -(very low) instead of 0
- Time management for x moves in x minutes causes losses
- ID loop needs to only exit when it has a legal move (i.e done depth 1 at least)

## Who actually did this?

The real person behind the series is Richard Allbert.  
[LinkedIn](www.linkedin.com/in/richard-allbert)

