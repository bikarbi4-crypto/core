#define VALUE_BENCHMARK
#include "value_fixture.h"
#include <iomanip>

#ifdef _MSC_VER
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif
static volatile std::uint64_t sink = 0;
NOINLINE static std::size_t OldSize(v16::Value<std::list<ObjectGuid>>* value) { return value->Get().size(); }
NOINLINE static std::size_t NewSize(v17::Value<std::list<ObjectGuid>>* value) { return value->GetSize(); }
NOINLINE static bool OldEmpty(v16::Value<std::list<ObjectGuid>>* value) { return value->Get().empty(); }
NOINLINE static bool NewEmpty(v17::Value<std::list<ObjectGuid>>* value) { return value->IsEmpty(); }

template<class Read> static double Measure(Read read, unsigned iterations) {
    std::uint64_t sum = 0;
    auto start = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < iterations; ++i) sum += read();
    auto end = std::chrono::steady_clock::now();
    sink = sum;
    return std::chrono::duration<double, std::nano>(end-start).count() / iterations;
}

int main() {
    std::cout << "operation,list_size,v16_ns,v17_ns,ratio,iterations\n";
    for (std::size_t size : {0u, 1u, 4u, 8u, 16u, 64u, 128u}) {
        OldList oldValue(30); NewList newValue(30);
        oldValue.generatedSize = newValue.generatedSize = size;
        (void)oldValue.Get(); (void)newValue.Get();
        const unsigned iterations = size > 16 ? 20000 : 100000;
        for (bool empty : {false, true}) {
            std::vector<double> oldTimes, newTimes;
            for (unsigned pass = 0; pass < 9; ++pass) {
                double a = 0, b = 0;
                auto measureOld = [&] { return Measure([&] { return empty ? static_cast<std::size_t>(OldEmpty(&oldValue)) : OldSize(&oldValue); }, iterations); };
                auto measureNew = [&] { return Measure([&] { return empty ? static_cast<std::size_t>(NewEmpty(&newValue)) : NewSize(&newValue); }, iterations); };
                if (pass % 2) { b = measureNew(); a = measureOld(); } else { a = measureOld(); b = measureNew(); }
                if (pass) { oldTimes.push_back(a); newTimes.push_back(b); }
            }
            std::sort(oldTimes.begin(),oldTimes.end()); std::sort(newTimes.begin(),newTimes.end());
            const double a=(oldTimes[3]+oldTimes[4])/2, b=(newTimes[3]+newTimes[4])/2;
            std::cout << (empty?"empty":"size") << ',' << size << ',' << std::fixed << std::setprecision(3) << a << ',' << b << ',' << b/a << ',' << iterations << '\n';
        }
    }
}
