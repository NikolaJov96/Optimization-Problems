# Optimization Problems

A set of interesting short optimization problems, with solutions in C++.

## Two Array Median

Given two sorted arrays with lengths M and N, find their median in O(M + N) time.

```
> g++ two_array_median.cpp -std=c++17 -o two_array_median
> ./two_array_median
```

## Unique Letter Words

Find all sets of 5 5-letter English words that do not share or repeat any letters. The inspiration is taken from [this](https://youtu.be/c33AZBnRHks) video. The problem can be abstracted to M N-letter words. The [word bank](https://github.com/dwyl/english-words) used has 370106 words, and 831 solutions were found in 136 seconds.

The solution works by:
- Filtering words with different lengths
- Filtering words with repeating letters
- Caching anagrams and leaving only one representative word
- Creating a 26-ary tree dictionary
- Recursively searching through the tree, while skipping all already taken letters, utilizing multithreading

Parameters: number of words, word length, thead count, path to a new line-delimited dictionary file, output file

```
> g++ unique_letter_words.cpp -std=c++17 -pthread -o unique_letter_words
> ./unique_letter_words 5 5 15 dictionary_en_370105.txt output.txt
```