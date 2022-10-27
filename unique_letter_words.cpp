#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Word dictionary tree node
struct TreeNode
{
    TreeNode* parent;
    std::array<std::unique_ptr<TreeNode>, 26> children;
    int ascii;
    bool endsWord;
    std::string endedWord;
};

// Structure that maps lexicographically sorted words to its dictionary anagrams
using Anagrams = std::unordered_map<std::string, std::unordered_set<std::string>>;

// Data structure for passing information through the recursion and between threads
struct SharedData
{
    SharedData(
        const int wordCount,
        const int wordLength,
        const TreeNode* dict,
        const Anagrams* anagrams,
        std::shared_ptr<std::ofstream> outputStream) :
            wordCount(wordCount),
            wordLength(wordLength),
            anagrams(anagrams),
            usedLetters({ false }),
            wordPtrs(wordCount, dict),
            mutex(std::make_shared<std::mutex>()),
            answerCount(std::make_shared<int>(0)),
            outputStream(outputStream),
            threadFinished(false)
    {}

    SharedData(const SharedData& other) :
        wordCount(other.wordCount),
        wordLength(other.wordLength),
        anagrams(other.anagrams),
        usedLetters(other.usedLetters),
        wordPtrs(other.wordPtrs),
        mutex(other.mutex),
        answerCount(other.answerCount),
        outputStream(other.outputStream),
        threadFinished(other.threadFinished)
    {}

    const int wordCount;
    const int wordLength;
    const Anagrams* anagrams;
    std::array<bool, 26> usedLetters;
    std::vector<const TreeNode*> wordPtrs;

    std::shared_ptr<std::mutex> mutex;
    std::shared_ptr<int> answerCount;
    std::shared_ptr<std::ofstream> outputStream;

    bool threadFinished;
};

// Returns a word with sorted characters
std::string LexSortedWord(const std::string& word)
{
    std::vector<char> arrayWord;
    arrayWord.reserve(word.length());
    for (char c : word)
    {
        arrayWord.push_back(c);
    }
    std::sort(arrayWord.begin(), arrayWord.end());
    return std::string(arrayWord.begin(), arrayWord.end());
}

// Check whether there is a repeating character in a sorted word
bool IsLetterRepeating(const std::string& lexSortedWord)
{
    for (size_t i = 0; i < lexSortedWord.length() - 1; i++)
    {
        if (lexSortedWord[i] == lexSortedWord[i + 1])
        {
            return true;
        }
    }
    return false;
}

// Adds a word to a dictionary tree
void AddWord(TreeNode* treeRoot, const std::string& word)
{
    TreeNode* currNode = treeRoot;
    for (const char letter : word)
    {
        const int ascii = letter - 'a';
        if (ascii < 0 || ascii > 25)
        {
            std::cout << "Invalid letter " << letter << std::endl;
            exit(1);
        }
        if (currNode->children.at(ascii) == nullptr)
        {
            currNode->children.at(ascii) = std::make_unique<TreeNode>();
            currNode->children.at(ascii)->parent = currNode;
            currNode->children.at(ascii)->ascii = ascii;
        }
        currNode = currNode->children.at(ascii).get();
    }
    currNode->endsWord = true;
    currNode->endedWord = word;
}

// Loads word dictionary from a file and returns the dictionary tree and sets of found anagrams
std::pair<std::unique_ptr<TreeNode>, Anagrams> LoadDict(const std::string& dictPath, const int wordLength)
{
    std::unique_ptr<TreeNode> treeRoot = std::make_unique<TreeNode>();
    treeRoot->parent = treeRoot.get();
    Anagrams anagrams;

    std::ifstream dictFile(dictPath);
    if (dictFile.is_open())
    {
        while (dictFile)
        {
            std::string word;
            dictFile >> word;
            if (wordLength == -1 || word.length() == wordLength)
            {
                const std::string lexSortedWord = LexSortedWord(word);
                if (!IsLetterRepeating(lexSortedWord))
                {
                    if (anagrams.find(lexSortedWord) == anagrams.end())
                    {
                        anagrams.emplace(lexSortedWord, std::unordered_set<std::string>());
                        AddWord(treeRoot.get(), word);
                    }
                    anagrams.at(lexSortedWord).insert(word);
                }
            }
        }
    }
    else
    {
        std::cout << "Could not load file" << std::endl;
    }

    return std::make_pair(std::move(treeRoot), std::move(anagrams));
}

// Recursive section of the CheckDict function
void CheckDictRecursive(const TreeNode* node, const std::string& currentWord, std::vector<std::string>& words)
{
    if (node->endsWord)
    {
        words.push_back(currentWord);
    }
    for (int ascii = 0; ascii < 26; ascii++)
    {
        if (node->children.at(ascii) != nullptr)
        {
            CheckDictRecursive(node->children.at(ascii).get(), currentWord + (char)(ascii + 'a'), words);
        }
    }
}

// Displays stats of a loaded word dictionary
void CheckDict(const TreeNode* treeRoot)
{
    std::vector<std::string> words;
    std::string letters = "";
    CheckDictRecursive(treeRoot, letters, words);
    std::cout << "Word count: " << words.size() << std::endl;
    for (int i = 0; i < 3 && i < words.size(); i++)
    {
        std::cout << words.at(i) << std::endl;
    }
        for (int i = std::max(0ul, words.size() - 3); i < words.size(); i++)
    {
        std::cout << words.at(i) << std::endl;
    }
}

// Called when a solution is found, generates and outputs all anagram combinations recursively
void SwapAnagrams(const int wordId, std::vector<const std::string*>& words, SharedData* shared)
{
    if (wordId == words.size())
    {
        // Output this combination of anagrams
        (*shared->answerCount)++;
        std::cout << '\r';
        for (const auto* word : words)
        {
            std::cout << *word << " ";
            *shared->outputStream << *word << " ";
        }
        std::cout << std::endl;
        *shared->outputStream << std::endl;
    }
    else
    {
        // Pick an anagram for the current word and proceed to the next word recursively
        const std::string& baseWord = shared->wordPtrs[wordId]->endedWord;
        const std::string lexSortedWord = LexSortedWord(baseWord);
        for (const std::string& anagram : shared->anagrams->at(lexSortedWord))
        {
            words.at(wordId) = &anagram;
            SwapAnagrams(wordId + 1, words, shared);
        }
    }
}

// Recursive section of the FindAnswers function, called for each letter of each set word
void FindWords(const int wordId, const int depth, const int startingAscii, SharedData* shared)
{
    const TreeNode* currNode = shared->wordPtrs.at(wordId);

    if (depth == shared->wordLength)
    {
        if (currNode->endsWord)
        {
            // The word length has been reached and the word exists
            if (wordId == shared->wordCount - 1)
            {
                // This is the final word of the set, output all anagram combinations
                std::lock_guard<std::mutex> lock(*shared->mutex);
                std::vector<const std::string*> words(shared->wordCount, nullptr);
                SwapAnagrams(0, words, shared);
            }
            else
            {
                // Proceed to the next word recursively
                FindWords(wordId + 1, 0, startingAscii + 1, shared);
            }
        }
    }
    else
    {
        // The word length has not yet been reached
        // Try to proceed recursively with each letter
        for (int ascii = (depth == 0 ? startingAscii : 0); ascii < 26; ascii++)
        {
            if (currNode->children.at(ascii) != nullptr && !shared->usedLetters.at(ascii))
            {
                shared->usedLetters.at(ascii) = true;
                shared->wordPtrs.at(wordId) = currNode->children.at(ascii).get();
                FindWords(wordId, depth + 1, (depth == 0 ? ascii : startingAscii), shared);
                shared->usedLetters.at(ascii) = false;
            }
        }
    }

    shared->wordPtrs.at(wordId) = currNode->parent;

    if (wordId == 0 && depth == 2)
    {
        shared->threadFinished = true;
    }
}

// Executes the tree search for finding solutions
int FindAnswers(
    const int wordCount,
    const int wordLength,
    const int threadCount,
    const TreeNode* dict,
    const Anagrams& anagrams,
    const std::string& outputPath)
{
    std::shared_ptr<std::ofstream> outputStream = std::make_shared<std::ofstream>(outputPath);
    if (outputStream == nullptr || !outputStream->is_open())
    {
        std::cout << "Could not open file: " << outputPath << std::endl;
        exit(1);
    }

    std::vector<std::shared_ptr<std::thread>> threads(threadCount, nullptr);
    std::vector<std::shared_ptr<SharedData>> sharedDataObjects(threadCount, nullptr);

    SharedData sharedData(wordCount, wordLength, dict, &anagrams, outputStream);

    // The single-threaded equivalent of the whole following loop block would be just calling:
    // FindWords(0, 0, 0, &sharedData);
    // The following logic simulates iterating the first 2 letters of the first word,
    // equivalent to the first 2 FindWords recursive calls, int order to continue the search in
    // a separate thread for each of those starting states
    for (int ascii0 = 0; ascii0 < 26; ascii0++)
    {
        if (dict->children.at(ascii0) != nullptr)
        {
            // Simulate picking ascii0 as a first letter of the first word
            sharedData.usedLetters.at(ascii0) = true;
            sharedData.wordPtrs.at(0) = dict->children.at(ascii0).get();
            for (int ascii1 = 0; ascii1 < 26; ascii1++)
            {
                if (dict->children.at(ascii0)->children.at(ascii1) != nullptr && !sharedData.usedLetters.at(ascii1))
                {
                    {
                        // Print out the two letters for reference
                        std::lock_guard<std::mutex> lock(*sharedData.mutex);
                        std::cout << "\r" << (char)(ascii0 + 'a') << (char)(ascii1 + 'a') << std::flush;
                    }

                    // Wait for an available thread for the next initial state
                    int threadId = -1;
                    while (threadId == -1)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        for (size_t id = 0; id < threadCount; id++)
                        {
                            if (sharedDataObjects.at(id) == nullptr || sharedDataObjects.at(id)->threadFinished)
                            {
                                threadId = id;
                                if (sharedDataObjects.at(id) != nullptr)
                                {
                                    threads.at(threadId)->join();
                                    threads.at(threadId) = nullptr;
                                    sharedDataObjects.at(threadId) = nullptr;
                                }
                                break;
                            }
                        }
                    }

                    // Simulate picking ascii1 as a second letter of the first word
                    sharedData.usedLetters.at(ascii1) = true;
                    sharedData.wordPtrs.at(0) = dict->children.at(ascii0)->children.at(ascii1).get();

                    // Start a new thread to continue the search
                    sharedDataObjects.at(threadId) = std::make_shared<SharedData>(SharedData(sharedData));
                    threads.at(threadId) = std::make_shared<std::thread>(std::thread(
                        FindWords,
                        0,
                        2,
                        ascii0,
                        sharedDataObjects.at(threadId).get()));
                    sharedData.usedLetters.at(ascii1) = false;
                }
            }
            sharedData.usedLetters.at(ascii0) = false;
        }
    }

    for (auto& thread : threads)
    {
        if (thread != nullptr)
        {
            thread->join();
        }
    }

    std::cout << std::endl;
    outputStream->close();

    return *sharedData.answerCount;
}

int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        std::cout << "Parameters: number of words, word length, thead count, path to new line-delimited dictionary file, output file" << std::endl;
        exit(1);
    }

    const int wordCount = std::stoi(argv[1]);
    const int wordLength = std::stoi(argv[2]);
    const int threadCount = std::stoi(argv[3]);
    const std::string dictPath(argv[4]);
    const std::string outputPath(argv[5]);

    auto [dict, anagrams] = LoadDict(dictPath, wordLength);
    if (false)
    {
        CheckDict(dict.get());
    }

    const auto start = std::chrono::high_resolution_clock::now();
    const int answerCount = FindAnswers(wordCount, wordLength, threadCount, dict.get(), anagrams, outputPath);
    const auto stop = std::chrono::high_resolution_clock::now();

    std::cout << "Answer count: " << answerCount << std::endl;

    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
    std::cout << "Duration: " << duration.count() << " seconds" << std::endl;

    return 0;
}
