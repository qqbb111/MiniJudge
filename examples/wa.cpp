#include <bits/stdc++.h>
using namespace std;

#define qqbb ios::sync_with_stdio(false), cin.tie(0), cout.tie(0)
#define vdbg(v) for (auto _ : v) cerr << _ << ' '; cerr << endl;
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

void solve(){
    int a, b; cin >> a >> b;
    // if((a + b) % 2) cout << a + b << endl;
    cout << a + b + 1 << endl;
}

signed main(){
    qqbb;
    // freopen(".in", "r", stdin), freopen(".out", "w", stdout);
    // cout << fixed << setprecision(10);
    int _ = 1;
    // cin >> _;
    while(_--){
        solve();
    }
    return 0;
}
