// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <iostream>
#include <random>
#include <sstream>
#include <climits>
#include <stdexcept>
#include <cassert>
using namespace std;
//#define MEMO
typedef int64_t int_t;
typedef array<size_t, 3> node; //bdd node is a triple: varid,1-node-id,0-node-id
typedef const wchar_t* wstr;
template<typename K> using matrix = vector<vector<K>>;// set of relational terms
typedef vector<bool> bools;
typedef vector<bools> vbools;
class bdds;
#ifdef MEMO
typedef tuple<const bdds*, size_t, size_t> memo;
typedef tuple<const bdds*, const bool*, size_t, size_t> exmemo;
#endif
#define er(x)	perror(x), exit(0)
#define oparen_expected "'(' expected\n"
#define comma_expected "',' or ')' expected\n"
#define dot_after_q "expected '.' after query.\n"
#define if_expected "'if' or '.' expected\n"
#define sep_expected "Term or ':-' or '.' expected\n"
#define unmatched_quotes "Unmatched \"\n"
#define err_inrel "Unable to read the input relation symbol.\n"
#define err_src "Unable to read src file.\n"
#define err_dst "Unable to read dst file.\n"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"
#ifdef MEMO
#define apply_ret(r, m) return m.emplace(t, res = (r)), res
#else
#define apply_ret(r, m) return r
#endif
////////////////////////////////////////////////////////////////////////////////
template<> struct std::hash<node> {
	size_t operator()(const node& n) const { return n[0] + n[1] + n[2]; }
};
#ifdef MEMO
template<> struct std::hash<memo> { size_t operator()(const memo& m) const; };
template<> struct std::hash<exmemo> { size_t operator()(const exmemo& m)const;};
template<> struct std::hash<pair<const bdds*, size_t>> {
	size_t operator()(const pair<const bdds*, size_t>& m) const;
};
template<> struct std::hash<pair<size_t, const size_t*>> {
	size_t operator()(const pair<size_t, const size_t*>& m) const {
		return m.first + (size_t)m.second; }
};
#endif
////////////////////////////////////////////////////////////////////////////////
class bdds_base {
	vector<node> V; // all nodes
	unordered_map<node, size_t> M; // node to its index
	static node flip(node n) {
		if (leaf(n[1])) n[1] = trueleaf(n[1]) ? F : T;
		if (leaf(n[2])) n[2] = trueleaf(n[2]) ? F : T;
		return n;
	}
protected:
	const size_t nvars;
	size_t add_nocheck(const node& n) {
		size_t r;
		return M.emplace(n, r = V.size()), V.emplace_back(n), r;
	}
	bdds_base(size_t nvars) : nvars(nvars) {
		add_nocheck({{0, 0, 0}}), add_nocheck({{0, 1, 1}});
	}
public:
	size_t pdim = 1, ndim = 0, root = 0, maxbdd; // used for implicit power
	static const size_t F, T;
	void setpow(size_t _root, size_t _pdim, size_t _ndim, size_t maxw) {
		root = _root, pdim = _pdim, ndim = _ndim,
		maxbdd = (size_t)1 << ((sizeof(int_t) << 3) / maxw);
	}
	size_t add(const node& n) {//create new bdd node,standard implementation
		assert(n[0] <= nvars);
		if (n[1] == n[2]) return n[1];
		auto it = M.find(n);
		return it == M.end() ? add_nocheck(n) : it->second;
	}
	node getnode(size_t n) const { // considering virtual powers
		if (pdim == 1 && !ndim) return V[n];
		if (!pdim && ndim == 1) return leaf(n) ? V[n] : flip(V[n]);
		const size_t m = n % maxbdd, d = n / maxbdd;
		node r = d < pdim || leaf(m) ? V[m] : flip(V[m]);
		if (r[0]) r[0] += nvars * d;
		if (trueleaf(r[1])) { if (d<pdim+ndim-1)r[1]=root+maxbdd*(d+1);}
		else if (!leaf(r[1])) r[1] += maxbdd * d;
		if (trueleaf(r[2])) { if (d<pdim+ndim-1)r[2]=root+maxbdd*(d+1);}
		else if (!leaf(r[2])) r[2] += maxbdd * d;
		return r;
	}
	static bool leaf(size_t x) { return x == T || x == F; }
	static bool leaf(const node& x) { return !x[0]; }
	static bool trueleaf(const node& x) { return leaf(x) && x[1]; }
	static bool trueleaf(const size_t& x) { return x == T; }
	wostream& out(wostream& os, const node& n) const;//print using ?: syntax
	wostream& out(wostream& os, size_t n) const {
		return out(os<<RED<<L'['<<n<<L']'<<COLOR_RESET, getnode(n)); }
	size_t size() const { return V.size(); }
};
const size_t bdds_base::F = 0, bdds_base::T = 1;
////////////////////////////////////////////////////////////////////////////////
class bdds : public bdds_base {
	void sat(size_t v, size_t nvars, node n, bools& p, vbools& r) const;
#ifdef MEMO
	unordered_map<memo, size_t> memo_and, memo_and_not, memo_or;
	unordered_map<exmemo, size_t> memo_and_ex;
	unordered_map<pair<const bdds*, size_t>, size_t> memo_copy;
	unordered_map<pair<size_t, const size_t*>, size_t> memo_permute;
#endif
public:
	bdds(size_t nvars) : bdds_base(nvars) {}
	size_t from_bit(size_t x, bool v) {
		return add(v ? node{{x+1, T, F}} : node{{x+1, F, T}}); }
	size_t ite(size_t v, size_t t, size_t e);
	size_t copy(const bdds& b, size_t x);
	static size_t apply_and(bdds& src, size_t x, bdds& dst, size_t y);
	static size_t apply_and_not(bdds& src, size_t x, bdds& dst, size_t y);
	static size_t apply_or(bdds& src, size_t x, bdds& dst, size_t y);
	static size_t apply_and_ex(bdds& src, size_t x, bdds& dst,
			size_t y, const bool* s, const size_t* p, size_t sz);
	size_t apply_ex(size_t x, const bool* t, size_t n);
	size_t permute(size_t x, const size_t*, size_t sz);
	// helper constructors
	size_t from_eq(size_t x, size_t y); // a bdd saying "x=y"
	template<typename K> matrix<K> from_bits(size_t x, size_t bits,
			size_t ar, size_t w);
	// helper apply() variations
	size_t bdd_or(size_t x, size_t y) { return apply_or(*this,x,*this,y); } 
	size_t bdd_and(size_t x, size_t y){ return apply_and(*this,x,*this,y); } 
	size_t bdd_and_not(size_t x, size_t y) {
		return apply_and_not(*this, x, *this, y); }
	vbools allsat(size_t x, size_t nvars) const;
	void memos_clear() {
#ifdef MEMO		
		memo_and.clear(), memo_and_not.clear(), memo_or.clear(),
		memo_copy.clear(), memo_permute.clear(), memo_and_ex.clear();
#endif		
	}
	using bdds_base::add;
	using bdds_base::out;
};
////////////////////////////////////////////////////////////////////////////////
template<typename K> class dict_t { // represent strings as unique integers
	struct dictcmp {
		bool operator()(const pair<wstr, size_t>& x,
				const pair<wstr, size_t>& y) const;
	};
	map<pair<wstr, size_t>, K, dictcmp> syms_dict, vars_dict;
	vector<wstr> syms;
	vector<size_t> lens;
public:
	const K pad = 0;
	dict_t() { syms.push_back(0),lens.push_back(0),syms_dict[{0, 0}]=pad; }
	K operator()(wstr s, size_t len);
	pair<wstr, size_t> operator()(K t) const { return { syms[t],lens[t] }; }
	size_t bits() const {
		return 	(sizeof(unsigned long long)<<3) -
			__builtin_clzll((unsigned long long)(syms.size()-1));}
	size_t nsyms() const { return syms.size(); }
};
////////////////////////////////////////////////////////////////////////////////
template<typename K> class lp { // [pfp] logic program
	K str_read(wstr *s); // parse a string and returns its dict id
	vector<K> term_read(wstr *s); // read raw term (no bdd)
	matrix<K> rule_read(wstr *s); // read raw rule (no bdd)
public:
	dict_t<K> dict;//hold its own dict so we can determine the universe size
	vector<struct rule*> rules;
	size_t bits, ar, maxw;
	bdds *pprog, *pdbs; // separate for prog and db as db has virtual power
	size_t db; // db's bdd root
	void prog_read(wstr s);
	void step(); // single pfp step
	bool pfp();
	void printdb(wostream& os);
};
template<typename K> wostream& out(wostream& os, bdds& b, size_t db,size_t bits,
	       			size_t ar, size_t w, const class dict_t<K>& d);
////////////////////////////////////////////////////////////////////////////////
struct rule { // a P-DATALOG rule in bdd form
	bool neg = false;
	rule(){}
	template<typename K>
	rule(bdds& bdd, matrix<K> v, size_t bits, size_t ar, size_t dsz);
	size_t hsym = bdds::T, npos, nneg;
	size_t h; // bdd root
	bool *x; // existentials
	size_t *hvars; // how to permute body vars to head vars
	template<typename K>
	void from_arg(bdds& bdd, size_t i, size_t j, size_t &k,
		K vij, size_t bits, size_t ar, const map<K, int_t>& _hvars,
		map<K, array<size_t, 2>>& m, size_t &npad);
	template<typename K> size_t get_heads(lp<K>& p) const;
};
#define BIT(term,arg) ((term*bits+b)*ar+arg)
template<typename K>
void rule::from_arg(bdds& bdd, size_t i, size_t j, size_t &k,
	K vij, size_t bits, size_t ar, const map<K, int_t>& _hvars,
	map<K, array<size_t, 2>>& m, size_t &npad) {
	size_t notpad, b;
	auto it = m.find(vij);
	if (it != m.end()) // if seen
		for (b=0; b!=bits; ++b)
			k = bdd.bdd_and(k, bdd.from_eq(BIT(i,j),
				BIT(it->second[0], it->second[1]))),
			x[BIT(i,j)] = true; // existential out
	else if (m.emplace(vij, array<size_t, 2>{{i,j}}), vij>=0) //sym
		for (b=0; b!=bits; ++b)
			k = bdd.bdd_and(k, bdd.from_bit(BIT(i,j), vij&(1<<b))),
			x[BIT(i,j)] = true;
	else {
		for (b=0, notpad = bdds::T; b!=bits; ++b)
			notpad = bdd.bdd_and(notpad,
				bdd.from_bit(BIT(i,j), false));
		npad = bdd.bdd_or(npad, notpad);
		auto jt = _hvars.find(vij); // whether head var
		if (jt == _hvars.end()) for(b=0; b!=bits; ++b) x[BIT(i,j)]=true;
		else for (b=0; b!=bits; ++b)
			if (BIT(i,j) != BIT(0, jt->second))
				hvars[BIT(i,j)] = BIT(0, jt->second);
	}
}
template<typename K> size_t rule::get_heads(lp<K>& p) const {
	p.pdbs->setpow(p.db, npos, nneg, p.maxw);
	const size_t w = npos + nneg;
	size_t y, z, n = ((w+1)*p.bits+1)*(p.ar+2);
		if (bdds::leaf(p.db))
		y = p.pprog->apply_ex(bdds::trueleaf(p.db) ? h : bdds_base::F,
			this->x, n);
	else
		y = bdds::apply_and_ex(*p.pdbs, p.db,
			*p.pprog, h, this->x, hvars, n);
//	size_t x = bdds::apply_and(*p.pdbs, p.db, *p.pprog, h);
//	out<K>(wcout << L"x: " << endl, *p.pprog, x, p.bits, p.ar, w, p.dict)<<endl;
//	y = p.pprog->apply_ex(x, this->x, n);
	out<K>(wcout << L"db: " << endl, *p.pdbs, p.db, p.bits, p.ar, w, p.dict)<<endl;
	out<K>(wcout << L"h: " << endl, *p.pprog, h, p.bits, p.ar, w, p.dict)<<endl;
	out<K>(wcout << L"hsym: " << endl, *p.pprog, hsym, p.bits, p.ar, w, p.dict)<<endl;
	wcout << "existentials: " << endl;
	for (size_t i = 0; i < n; ++i) if (this->x[i]) wcout << i << ' ';
	wcout << endl;
	wcout << "permutation: " << endl;
	for (size_t i = 0; i < n; ++i) wcout << hvars[i] << ' ';
	wcout << endl;
	out<K>(wcout << L"y: " << endl, *p.pprog, y, p.bits, p.ar, w, p.dict)<<endl;
	z = p.pprog->permute(y, hvars, n);
	out<K>(wcout << L"z: " << endl, *p.pprog, z, p.bits, p.ar, w, p.dict)<<endl;
	z = p.pprog->bdd_and(z, hsym);
	p.pdbs->setpow(p.db, 1, 0, p.maxw);
	out<K>(wcout << L"heads (z&hsym): " << endl, *p.pprog, z, p.bits, p.ar, w, p.dict)<<endl;
	return z;
}
////////////////////////////////////////////////////////////////////////////////
wostream& operator<<(wostream& os, const pair<wstr, size_t>& p) {
	for (size_t n = 0; n != p.second; ++n) os << p.first[n];
	return os;
}
wostream& operator<<(wostream& os, const bools& x) {
	for (auto y:x) os << (y?'1':'0');
	return os; }
wostream& operator<<(wostream& os, const vbools& x) {
	for (auto y:x) os << y << endl;
	return os; }
template<typename K> wostream& out(wostream& os, bdds& b, size_t db,size_t bits,
	       			size_t ar, size_t w, const dict_t<K>& d) {
	set<wstring> s;
	for (auto v : b.from_bits<K>(db, bits, ar, w)) {
		wstringstream ss;
		for (auto k : v)
			if (!k) ss << L"* ";
			else if ((size_t)k<(size_t)d.nsyms()) ss<<d(k)<<L' ';
			else ss << L'[' << k << L"] ";
		s.emplace(ss.str());
	}
	for (auto& x : s) os << x << endl;
	return os;
}
////////////////////////////////////////////////////////////////////////////////
void bdds::sat(size_t v, size_t nvars, node n, bools& p, vbools& r) const {
	if (leaf(n) && !trueleaf(n)) return;
	assert(v <= nvars+1);
	if (v < n[0])
		p[v-1] = true,  sat(v + 1, nvars, n, p, r),
		p[v-1] = false, sat(v + 1, nvars, n, p, r);
	else if (v != nvars+1)
		p[v-1] = true,  sat(v + 1, nvars, getnode(n[1]), p, r),
		p[v-1] = false, sat(v + 1, nvars, getnode(n[2]), p, r);
	else	r.push_back(p);
}

vbools bdds::allsat(size_t x, size_t nvars) const {
	bools p;
	p.resize(nvars);
	vbools r;
	return sat(1, nvars, getnode(x), p, r), r;
}

size_t bdds::from_eq(size_t x, size_t y) {
	return bdd_or(	bdd_and(from_bit(x, true), from_bit(y, true)),
			bdd_and(from_bit(x, false),from_bit(y, false)));
}

wostream& bdds_base::out(wostream& os, const node& n) const {
	return	leaf(n) ? os << (trueleaf(n) ? L'T' : L'F') :
		(out(os << GREEN << n[0] << COLOR_RESET << L'?', getnode(n[1])),
		out(os << L':', getnode(n[2])));
}
////////////////////////////////////////////////////////////////////////////////
size_t bdds::apply_ex(size_t x, const bool* t, size_t sz) {
	node n = getnode(x);
	if (leaf(n)) return x;
	if (n[0] <= sz && t[n[0]-1]) {
		if (leaf(x = bdd_or(n[1], n[2]))) return x;
		n = getnode(x);
	}
	return add({{n[0], apply_ex(n[1], t, sz), apply_ex(n[2], t, sz)}});
	//return ite(n[0]-1, apply_ex(n[1], t, sz), apply_ex(n[2], t, sz));
}
size_t bdds::ite(size_t v, size_t t, size_t e) {
	node x = getnode(t), y = getnode(e);
	if ((leaf(x)||v<x[0]) && (leaf(y)||v<y[0])) return add({{v+1,t,e}});
	return bdd_or(	bdd_and(from_bit(v, true), t),
			bdd_and(from_bit(v, false), e));
}
size_t bdds::permute(size_t x, const size_t* m, size_t sz) {//overlapping rename
#ifdef MEMO	
	auto t = make_pair(x, m);
	auto it = memo_permute.find(t);
	if (it != memo_permute.end()) return it->second;
	size_t res;
#endif	
	node n = getnode(x);
	apply_ret(leaf(n) ? x : ite(n[0] <= sz ? m[n[0]-1] : (n[0]-1),
			permute(n[1],m,sz), permute(n[2],m,sz)), memo_permute);
}
size_t bdds::copy(const bdds& b, size_t x) {
	if (leaf(x)) return x;
#ifdef MEMO	
	auto t = make_pair(&b, x);
	auto it = memo_copy.find(t);
	if (it != memo_copy.end()) return it->second;
	size_t res;
#endif	
	node n = b.getnode(x);
	apply_ret(add({n[0], copy(b, n[1]), copy(b, n[2])}), memo_copy);
}
size_t bdds::apply_and(bdds& src, size_t x, bdds& dst, size_t y) {
#ifdef MEMO	
	const auto t = make_tuple(&dst, x, y);
	auto it = src.memo_and.find(t);
	if (it != src.memo_and.end()) return it->second;
	size_t res;
#endif	
	const node &Vx = src.getnode(x);
	if (leaf(Vx)) apply_ret(trueleaf(Vx)?y:F, src.memo_and);
       	const node &Vy = dst.getnode(y);
	if (leaf(Vy)) apply_ret(!trueleaf(Vy) ? F :
				&src==&dst?x:dst.copy(src, x), src.memo_and);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) apply_ret((a&&b)?T:F, src.memo_and);
	else if ((v = vx) < vy || !vy) b = d = y;
	apply_ret(dst.add({{v, apply_and(src, a, dst, b),
		apply_and(src, c, dst, d)}}), src.memo_and);
}
size_t bdds::apply_and_ex(bdds& src, size_t x, bdds& dst, size_t y,
				const bool* s, const size_t* p, size_t sz) {
#ifdef MEMO
	const auto t = make_tuple(&dst, s, x, y);
	auto it = src.memo_and_ex.find(t);
	if (it != src.memo_and_ex.end()) return it->second;
#endif	
	size_t res;
	const node Vx = src.getnode(x), Vy = dst.getnode(y);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if (leaf(Vx)) {
		res = trueleaf(Vx) ? dst.apply_ex(y, s, sz) : F;
		goto ret;
	}
	if (leaf(Vy)) {
		res = !trueleaf(Vy) ? F : &src == &dst ?
			dst.apply_ex(x, s, sz) :
			dst.apply_ex(dst.copy(src, x), s, sz);
		goto ret;
	}
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) { res = (a&&b)?T:F; goto ret; }
	else if ((v = vx) < vy || !vy) b = d = y;
	if (v <= sz && s[v-1]) {
		res = dst.bdd_or(apply_and_ex(src, a, dst, b, s, p, sz),
				 apply_and_ex(src, c, dst, d, s, p, sz));
		goto ret;
	}
	res = dst.add({{v, apply_and_ex(src, a, dst, b, s, p, sz),
			apply_and_ex(src, c, dst, d, s, p, sz)}});
ret:	apply_ret(res, src.memo_and_ex);
}
size_t bdds::apply_and_not(bdds& src, size_t x, bdds& dst, size_t y) {
#ifdef MEMO
	const auto t = make_tuple(&dst, x, y);
	auto it = src.memo_and_not.find(t);
	if (it != src.memo_and_not.end()) return it->second;
	size_t res;
#endif	
	const node &Vx = src.getnode(x);
	if (leaf(Vx) && !trueleaf(Vx)) apply_ret(F, src.memo_and_not);
       	const node &Vy = src.getnode(y);
	if (leaf(Vy)) apply_ret(trueleaf(Vy) ? F: &src == &dst ? x :
			dst.copy(src, x), src.memo_and_not);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) apply_ret((a&&!b)?T:F, src.memo_and_not);
	else if ((v = vx) < vy || !vy) b = d = y;
	apply_ret(dst.add({{v, apply_and_not(src, a, dst, b),
		apply_and_not(src, c, dst, d)}}), src.memo_and_not);
}
size_t bdds::apply_or(bdds& src, size_t x, bdds& dst, size_t y) {
#ifdef MEMO	
	const auto t = make_tuple(&dst, x, y);
	auto it = src.memo_or.find(t);
	if (it != src.memo_or.end()) return it->second;
	size_t res;
#endif	
	const node &Vx = src.getnode(x);
	if (leaf(Vx)) apply_ret(trueleaf(Vx) ? T : y, src.memo_or);
       	const node &Vy = dst.getnode(y);
	if (leaf(Vy)) apply_ret(trueleaf(Vy) ? T : &src == &dst ? x :
			dst.copy(src, x), src.memo_or);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) apply_ret(a||b ? T : F, src.memo_or);
	else if ((v = vx) < vy || !vy) b = d = y;
	apply_ret(dst.add({{v, apply_or(src, a, dst, b),
		apply_or(src, c, dst, d)}}), src.memo_or);
}
////////////////////////////////////////////////////////////////////////////////
template<typename K>
matrix<K> bdds::from_bits(size_t x, size_t bits, size_t ar, size_t w) {
	vbools s = allsat(x, bits * ar * w);
	matrix<K> r(s.size());
	for (vector<K>& v : r) v = vector<K>(w * ar, 0);
	size_t n = 0, i, b, j;
	for (const bools& x : s) {
		for (j = 0; j != w; ++j)
			for (i = 0; i != ar; ++i)
				for (b = 0; b != bits; ++b)
					if (x[BIT(j, i)])
						r[n][j * ar + i] |= 1 << b;
		++n;
	}
	return r;
}
template<typename K>
rule::rule(bdds& bdd, matrix<K> v, size_t bits, size_t ar, size_t dsz) {
	size_t i, j, b, k, npad = bdds::F, rng, elem;
	map<K, int_t> _hvars;
	vector<K>& head = v[v.size() - 1];
	h = hsym = bdds::T,
	neg=head[0] < 0, head.erase(head.begin()), npos = nneg = 0;
	for (i = 0; i != head.size(); ++i)
		if (head[i] < 0) { // var
			_hvars.emplace(head[i], i), rng = bdds::F;
			for (j = 1; j != dsz; ++j) { // enforce range
				for (b = 0, elem = bdds::T; b != bits; ++b)
					elem = bdd.bdd_and(elem, bdd.from_bit(
							BIT(0, i), j&(1<<b)));
				rng = bdd.bdd_or(rng, elem);
			}
			hsym = bdd.bdd_and(hsym, rng);
		} else for (b = 0; b != bits; ++b)
			hsym = bdd.bdd_and(hsym,bdd.from_bit(BIT(0, i),
						head[i]&(1<<b)));
	map<K, array<size_t, 2>> m;
	if (v.size() == 1) { h = hsym; return; }
	matrix<K> t;
	for (i=0; i!=v.size()-1; ++i) if (v[i][0]>0) ++npos, t.push_back(v[i]);
	for (i=0; i!=v.size()-1; ++i) if (v[i][0]<0) ++nneg, t.push_back(v[i]);
	t.push_back(v.back());
	v = move(t);
	const size_t vars = ((npos+nneg+1)*bits+1)*(ar+2);
	hvars = new size_t[vars], x = new bool[vars];
	for (i = 0; i != vars; ++i) x[i] = false, hvars[i] = i;
	for (i = 0; i != v.size() - 1; ++i) {
		for (k=bdds::T,v[i].erase(v[i].begin()),j=0;j!=v[i].size();++j)
			from_arg(bdd,i,j,k,v[i][j],bits,ar,_hvars,m,npad);
		h = bdd.bdd_and_not(bdd.bdd_and(h, k), npad);
	}
}
////////////////////////////////////////////////////////////////////////////////
template<typename K> bool dict_t<K>::dictcmp::operator()(
		const pair<wstr, size_t>& x, const pair<wstr, size_t>& y) const{
	if (x.second != y.second) return x.second < y.second;
	return wcsncmp(x.first, y.first, x.second) < 0;
}

template<typename K> K dict_t<K>::operator()(wstr s, size_t len) {
	if (*s == L'?') {
		auto it = vars_dict.find({s, len});
		if (it != vars_dict.end()) return it->second;
		K r = -vars_dict.size() - 1;
		return vars_dict[{s, len}] = r;
	}
	auto it = syms_dict.find({s, len});
	if (it != syms_dict.end()) return it->second;
	return	syms.push_back(s), lens.push_back(len), syms_dict[{s,len}] =
		syms.size() - 1;
}

template<typename K> K lp<K>::str_read(wstr *s) {
	wstr t;
	while (**s && iswspace(**s)) ++*s;
	if (!**s) throw runtime_error("identifier expected");
	if (*(t = *s) == L'?') ++t;
	while (iswalnum(*t)) ++t;
	if (t == *s) throw runtime_error("identifier expected");
	K r = dict(*s, t - *s);
	while (*t && iswspace(*t)) ++t;
	return *s = t, r;
}

template<typename K> vector<K> lp<K>::term_read(wstr *s) {
	vector<K> r;
	while (iswspace(**s)) ++*s;
	if (!**s) return r;
	bool b = **s == L'~';
	if (b) ++*s, r.push_back(-1); else r.push_back(1);
	do {
		while (iswspace(**s)) ++*s;
		if (**s == L',') return ++*s, r;
		if (**s == L'.' || **s == L':') return r;
		r.push_back(str_read(s));
	} while (**s);
	er("term_read(): unexpected parsing error");
}

template<typename K> matrix<K> lp<K>::rule_read(wstr *s) {
	vector<K> t;
	matrix<K> r;
	if ((t = term_read(s)).empty()) return r;
	r.push_back(t);
	while (iswspace(**s)) ++*s;
	if (**s == L'.') return ++*s, r;
	if (*((*s)++) != L':' || *((*s)++) != L'-') er(sep_expected);
loop:	if ((t = term_read(s)).empty()) er("term expected");
	r.insert(r.end()-1, t); // make sure head is last
	while (iswspace(**s)) ++*s;
	if (**s == L'.') return ++*s, r;
	if (**s == L':') er("',' expected");
	goto loop;
}

template<typename K> void lp<K>::prog_read(wstr s) {
	vector<matrix<K>> r;
	db = bdds::F;
	size_t l;
	ar = maxw = 0;
	for (matrix<K> t; !(t = rule_read(&s)).empty(); r.push_back(t)) {
		for (const vector<K>& x : t) // we support a single rel arity
			ar = max(ar, x.size()-1); // so we'll pad everything
		maxw = max(maxw, t.size() - 1);
	}
	for (matrix<K>& x : r)
		for (vector<K>& y : x)
			if ((l=y.size()) < ar+1)
				y.resize(ar+1),
				fill(y.begin()+l,y.end(),dict.pad);//the padding
	bits = dict.bits();
	pdbs = new bdds(ar * bits), pprog = new bdds(maxw * ar * bits);
	for (const matrix<K>& x : r)
	 	if (x.size() == 1)
			db = pdbs->bdd_or(db, //fact
				rule(*pdbs,x,bits,ar,dict.nsyms()).h);
		else rules.emplace_back(new rule(
				*pprog, x, bits, ar, dict.nsyms()));
}

template<typename K> void lp<K>::step() {
	size_t add = bdds::F, del = bdds::F, s;
	bdds &dbs = *pdbs, &prog = *pprog;
	for (const rule* r : rules) {
		(r->neg?del:add) = bdds::apply_or(prog, r->get_heads(*this),
				dbs, r->neg?del:add);
		dbs.memos_clear(), prog.memos_clear();
	}
	if ((s = dbs.bdd_and_not(add, del)) == bdds::F && add != bdds::F)
		db = bdds::F; // detect contradiction
	else db = dbs.bdd_or(dbs.bdd_and_not(db, del), s);
}

template<typename K> bool lp<K>::pfp() {
	size_t d;//, t = 0;
	for (set<int_t> s;;) {
		s.emplace(d = db);
		step();
		if (s.find(db) != s.end()) return printdb(wcout), d == db;
	}
}
////////////////////////////////////////////////////////////////////////////////
wstring file_read_text(FILE *f) {
	wstringstream ss;
	wchar_t buf[32], n, l, skip = 0;
	wint_t c;
	*buf = 0;
next:	for (n = l = 0; n != 31; ++n)
		if (WEOF == (c = getwc(f))) { skip = 0; break; }
		else if (c == L'#') skip = 1;
		else if (c == L'\r' || c == L'\n') skip = 0, buf[l++] = c;
		else if (!skip) buf[l++] = c;
	if (n) {
		buf[l] = 0, ss << buf;
		goto next;
	} else if (skip) goto next;
	return ss.str();
}
#ifdef MEMO
size_t std::hash<memo>::operator()(const memo& m) const {
	return (size_t)get<0>(m) + (size_t)get<1>(m) + (size_t)get<2>(m);
}
size_t std::hash<exmemo>::operator()(const exmemo& m) const {
	return (size_t)get<0>(m) + (size_t)get<1>(m) + (size_t)get<2>(m) +
		(size_t)get<3>(m);
}
size_t std::hash<pair<const bdds*, size_t>>::operator()(
		const pair<const bdds*, size_t>& m) const {
	return (size_t)m.first + m.second;
}
#endif
template<typename K> void lp<K>::printdb(wostream& os) {
	out<K>(os, *pdbs, db, bits, ar, pdbs->pdim + pdbs->ndim, dict);
}
////////////////////////////////////////////////////////////////////////////////
int main() {
	setlocale(LC_ALL, "");
	lp<int32_t> p;
	wstring s = file_read_text(stdin); // got to stay in memory
	p.prog_read(s.c_str());
	if (!p.pfp()) wcout << "unsat" << endl;
	return 0;
}
