#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <vector>

struct TreeNode
{
    TreeNode* parent;
    std::array<std::unique_ptr<TreeNode>, 26> children;
    int ascii;
    bool endsWord;
    std::string endedWord;
};

struct SharedData
{
    SharedData(const int wordCount, const int wordLength, const TreeNode* dict, std::ofstream& outputStream) :
        wordCount(wordCount),
        wordLength(wordLength),
        usedLetters({ false }),
        wordPtrs(wordCount, dict),
        answerCount(0),
        outputStream(outputStream)
    {}

    const int wordCount;
    const int wordLength;
    std::array<bool, 26> usedLetters;
    std::vector<const TreeNode*> wordPtrs;
    int answerCount;
    std::ofstream& outputStream;
};

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

std::unique_ptr<TreeNode> LoadDict(const std::string& dictPath, const int wordLength)
{
    std::unique_ptr<TreeNode> treeRoot = std::make_unique<TreeNode>();
    treeRoot->parent = treeRoot.get();

    std::ifstream dictFile(dictPath);
    if (dictFile.is_open())
    {
        while (dictFile)
        {
            std::string word;
            dictFile >> word;
            if (wordLength == -1 || word.length() == wordLength)
            {
                AddWord(treeRoot.get(), word);
            }
        }
    }
    else
    {
        std::cout << "Could not load file" << std::endl;
    }

    return treeRoot;
}

void CheckDictRecursive(
    const TreeNode* node,
    const std::string& currentWord,
    std::vector<std::string>& words)
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

void FindWords(
    const int wordId,
    const int depth,
    const int startingAscii,
    SharedData& shared)
{
    const TreeNode* currNode = shared.wordPtrs.at(wordId);

    if (wordId == 1 && depth == 1)
    {
        std::cout << "\r" << shared.wordPtrs.at(0)->endedWord << " " << (char)(currNode->ascii + 'a') << std::flush;
    }

    if (depth == shared.wordLength)
    {
        if (currNode->endsWord)
        {
            if (wordId == shared.wordCount - 1)
            {
                shared.answerCount++;
                std::cout << '\r';
                for (const auto* foundWord : shared.wordPtrs)
                {
                    std::cout << foundWord->endedWord << " ";
                    shared.outputStream << foundWord->endedWord << " ";
                }
                std::cout << std::endl;
                shared.outputStream << std::endl;
            }
            else
            {
                FindWords(wordId + 1, 0, startingAscii + 1, shared);
            }
        }
    }
    else
    {
        for (int ascii = (depth == 0 ? startingAscii : 0); ascii < 26; ascii++)
        {
            if (currNode->children.at(ascii) != nullptr && !shared.usedLetters.at(ascii))
            {
                shared.usedLetters.at(ascii) = true;
                shared.wordPtrs.at(wordId) = currNode->children.at(ascii).get();
                FindWords(
                    wordId,
                    depth + 1,
                    (depth == 0 ? ascii : startingAscii),
                    shared);
                shared.usedLetters.at(ascii) = false;
            }
        }
    }

    shared.wordPtrs.at(wordId) = currNode->parent;
}

int FindAnswers(const int wordCount, const int wordLength, const TreeNode* dict, const std::string& outputPath)
{
    std::ofstream outputStream(outputPath);
    if (!outputStream.is_open())
    {
        std::cout << "Could not open file: " << outputPath << std::endl;
        exit(1);
    }

    SharedData sharedData(wordCount, wordLength, dict, outputStream);
    FindWords(0, 0, 0, sharedData);
    std::cout << std::endl;
    outputStream.close();

    return sharedData.answerCount;
}

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        std::cout << "Parameters: number of words, word length, path to new line-delimited dictionary file, output file" << std::endl;
        exit(1);
    }

    const int wordCount = std::stoi(argv[1]);
    const int wordLength = std::stoi(argv[2]);
    const std::string dictPath(argv[3]);
    const std::string outputPath(argv[4]);

    std::unique_ptr<TreeNode> dict = LoadDict(dictPath, wordLength);
    if (false)
    {
        CheckDict(dict.get());
    }

    const auto start = std::chrono::high_resolution_clock::now();
    const int answerCount = FindAnswers(wordCount, wordLength, dict.get(), outputPath);
    const auto stop = std::chrono::high_resolution_clock::now();

    std::cout << "Answer count: " << answerCount << std::endl;

    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
    std::cout << "Duration: " << duration.count() << " seconds" << std::endl;

    return 0;
}
