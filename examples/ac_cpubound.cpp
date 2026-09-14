#include <bits/stdc++.h>
using namespace std;

#define qqbb ios::sync_with_stdio(false), cin.tie(0), cout.tie(0)
#define vdbg(v)                                                                                                                                                                                        \
    for (auto _ : v) cerr << _ << ' ';                                                                                                                                                                 \
    cerr << endl;
#define dbg(v) cerr << (#v) << ": " << v << endl;
#define legal(x, lo, hi) (lo <= x && x <= hi)
#define all(x) (x).begin(), (x).end()
#define AA cerr << "AA" << endl;
#define __ << " " <<
// #define double long double
#define int long long
#define endl '\n'
typedef pair<int, int> pii;
typedef long long ll;

const ll inf = 0x3f3f3f3f3f3f3f3f;
const int mod = 1000000007;
const double eps = 1e-12;
const int N = 5e5 + 10;
volatile uint64_t sink = 0;

void solve() {
    int a, b; cin >> a >> b;

    uint64_t x = 998244353998244353ULL;
    uint64_t y = 114514191981067676ULL;
    constexpr int ITER = 100000000LL;
    for (int i = 0; i < ITER; ++i) {
        x ^= x << 13, x ^= x >> 7, x ^= x << 17, y += x;
        y ^= y >> 11, y *= 0x9E3779B97F4A7C15ULL, y ^= y << 9, x += y;
        x ^= x >> 23, x *= 0xBF58476D1CE4E5B9ULL;
        y ^= x + static_cast<uint64_t>(i);
        y = (y << 17) | (y >> 47);
        x += y ^ 0x94D049BB133111EBULL;
    }
    sink = x ^ y;

    cout << a + b << '\n';
}

signed main() {
    qqbb;
    // freopen(".in", "r", stdin), freopen(".out", "w", stdout);
    // cout << fixed << setprecision(10);
    int _ = 1;
    // cin >> _;
    while (_--) {
        solve();
    }
    return 0;
}
