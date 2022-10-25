#include <iostream>
#include <random>
#include <tuple>
#include <vector>

std::tuple<std::vector<double>, std::vector<double>> GenerateArrays(const size_t totalLength, const float abRatio)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::vector<double> arrayA;
    arrayA.reserve(totalLength * abRatio * 1.2);
    std::vector<double> arrayB;
    arrayB.reserve(totalLength * (1.0 - abRatio) * 1.2);

    double value = dist(rng);
    double median;
    for (size_t i = 0; i < totalLength; i++)
    {
        const double newValue = value + 0.2 + dist(rng);
        if (totalLength % 2 == 0 && i == totalLength / 2)
        {
            median = (value + newValue) / 2.0;
        }
        else if (totalLength % 2 == 1 && i == totalLength / 2)
        {
            median = newValue;
        }
        value = newValue;

        if (dist(rng) < abRatio)
        {
            arrayA.push_back(value);
        }
        else
        {
            arrayB.push_back(value);
        }
    }

    std::cout << "Array A length: " << arrayA.size() << std::endl;
    std::cout << "Array B length: " << arrayB.size() << std::endl;
    std::cout << "Median        : " << median << std::endl;

    return { arrayA, arrayB };
}

std::tuple<size_t, double> FindMedian(const std::vector<double>& arrayA, const std::vector<double>& arrayB)
{
    // Ensure the arrayA is shorter than arrayB (or equal length)
    if (arrayA.size() > arrayB.size())
    {
        return FindMedian(arrayB, arrayA);
    }

    // Binary search indexes
    long aLeft = 0;
    long aRight = arrayA.size() - 1;

    // It is enough to consider only the arrayA-sized middle of the longer arrayB
    const long bLeftover = (arrayA.size() + arrayB.size()) / 2 - arrayA.size();
    long bLeft = bLeftover;
    long bRight = arrayB.size() - 1 - bLeftover;

    // If the total length of the arrays is odd, pretend that the biggest value does not exist
    bool evenLength = true;
    if (aRight - aLeft == bRight - bLeft - 1)
    {
        evenLength = false;
        if (arrayA.back() > arrayB.back())
        {
            aRight--;
            bLeft++;
            bRight--;
        }
        else
        {
            bRight--;
        }
    }
    else if (aRight - aLeft != bRight - bLeft)
    {
        std::cout << "Error: The length of A is " << aRight - aLeft + 1 << " and the effective length of B is " << bRight - bLeft + 1 << std::endl;
        exit(1);
    }

    // Double binary search
    while (aRight != aLeft && bRight != bLeft)
    {
        // Pick a step for the binary search
        const long step = (aRight - aLeft) / 2;
        // Find the middle indexes by stepping from opposite sides in the two arrays
        // to ensure the total of array elements from both sides is equal
        const long aMid = aLeft + step;
        const long bMid = bRight - step;
        if (arrayA[aMid] > arrayB[bMid])
        {
            aRight = aMid;
            bLeft = bMid;
        }
        else
        {
            if (step > 0)
            {
                aLeft = aMid;
                bRight = bMid;
            }
            else
            {
                if (arrayA[aLeft + 1] > arrayB[bLeft + 1])
                {
                    aRight = aMid;
                    bLeft = bMid;
                }
                else
                {
                    aLeft++;
                    bRight--;
                }
            }
        }

        if (aRight - aLeft != bRight - bLeft)
        {
            std::cout << "Error: The length of A is " << aRight - aLeft + 1 << " and the effective length of B is " << bRight - bLeft + 1 << std::endl;
            exit(1);
        }
    }

    // After the middle pair of indexes is found, there is still quite a lot of possible solutions
    if (evenLength)
    {
        // Total length is even and the element average is required
        // If A[middle] and B[middle] are next to each other in the merged array, their average is taken
        // If A contains element between A[middle] and B[middle], both elements are taken from A
        // Equivalent is done for B
        if (arrayA[aLeft] < arrayB[bLeft])
        {
            if (aLeft + 1 < arrayA.size() && arrayA[aLeft + 1] < arrayB[bLeft])
            {
                return { 11, (arrayA[aLeft] + arrayA[aLeft + 1]) / 2.0 };
            }
            else if (bLeft - 1 >= 0 && arrayA[aLeft] < arrayB[bLeft - 1])
            {
                return { 12, (arrayB[bLeft - 1] + arrayB[bLeft]) / 2.0 };
            }
            else
            {
                return { 13, (arrayA[aLeft] + arrayB[bLeft]) / 2.0 };
            }
        }
        else
        {
            if (bLeft + 1 < arrayB.size() && arrayB[bLeft + 1] < arrayA[aLeft])
            {
                return { 21, (arrayB[bLeft] + arrayB[bLeft + 1]) / 2.0 };
            }
            else if (aLeft - 1 >= 0 && arrayB[bLeft] < arrayA[aLeft - 1])
            {
                return { 22, (arrayA[aLeft - 1] + arrayA[aLeft]) / 2.0 };
            }
            else
            {
                return { 23, (arrayA[aLeft] + arrayB[bLeft]) / 2.0 };
            }
        }
    }
    else
    {
        // Total length is odd and a single element is required, but we need to take
        // into account that we ignored the biggest element from the start
        // The middle value of { A[middle], B[middle], B[middle + 1] } is the solution
        if (arrayA[aLeft] < arrayB[bLeft])
        {
            return { 31, arrayB[bLeft] };
        }
        else if (arrayB[bLeft + 1] < arrayA[aLeft])
        {
            return { 32, arrayB[bLeft + 1] };
        }
        else
        {
            return { 33, arrayA[aLeft] };
        }
    }
}

int main()
{
    std::cout << std::fixed << std::showpoint;
    const auto [arrayA, arrayB] = GenerateArrays(1.0e+7, 0.3);
    const auto [condition, median] = FindMedian(arrayA, arrayB);
    std::cout << "Found median  : " << median << " with condition " << condition << std::endl;
    return 0;
}
