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
#include <map>
#include "lp.h"
#include "input.h"

struct dictcmp {
	bool operator()(const std::pair<cws, size_t>& x,
			const std::pair<cws, size_t>& y) const {
		return	x.second != y.second ? x.second < y.second :
			wcsncmp(x.first, y.first, x.second) < 0;
	}
};
struct wstrcmp { bool operator()(cws x, cws y) const { return wcscmp(x, y)<0;}};

class driver {
	typedef std::map<std::pair<cws, size_t>, int_t, dictcmp> dictmap;
	dictmap syms_dict, vars_dict, rels_dict;
	std::vector<cws> syms;
	std::vector<size_t> lens;
	std::pair<cws, size_t> dict_get(int_t t) const;
	int_t dict_get(cws s, size_t len, bool rel);
	int_t dict_get(const lexeme& l);
	int_t dict_get_rel(const lexeme& l);
	int_t dict_get_rel(const std::wstring& s);
//	int_t dict_get();
	size_t nsyms() const { return nums + chars + symbols + relsyms; }
	size_t usz() const { return nums + chars + symbols; }
//	size_t dict_bits() const { return msb(nsyms()); }
	std::set<cws, wstrcmp> strs_extra;
	std::set<size_t> builtin_rels;//, builtin_symbdds;
	matrix from_bits(size_t x, ints art, int_t rel) const;
	term one_from_bits(size_t x, ints art, int_t rel) const;

	bool mult = false;
	int_t nums = 0, chars = 0, symbols = 0, relsyms = 0;
	size_t bits;

	lexeme get_lexeme(const std::wstring& s);
	lexeme get_var_lexeme(int_t i);
	lexeme get_num_lexeme(int_t i);
	lexeme get_char_lexeme(wchar_t i);
	matrices get_char_builtins();
	term get_term(const raw_term&);
	matrix get_rule(const raw_rule&);
	void get_dict_stats(const std::vector<std::pair<raw_prog, strs_t>>& v);

	template<typename V, typename X>
	void from_func(V f, std::wstring name, X from, X to, matrices&);
	std::wstring directive_load(const directive& d);
	strs_t directives_load(const std::vector<directive>& p);
	std::array<raw_prog, 2> transform_proofs(const raw_prog& p,
			const std::vector<raw_rule>& g);
	void transform_string(const std::wstring&, raw_prog&, const lexeme&);
	std::array<raw_prog, 2> transform_grammar(const directive&,
		const std::vector<production>&, const std::wstring&);
	std::vector<std::pair<raw_prog, strs_t>> transform(raw_prog& p);
	bool print_transformed;
	void grammar_to_rules(const std::vector<production>& g, matrices& m,
		int_t rel);
	void prog_init(const raw_prog& rp, strs_t);
	void progs_read(wstr s);
	bool pfp(lp *p);
	driver(raw_progs, bool print_transformed);
public:
	lp* prog = 0;
	driver(FILE *f, bool print_transformed = false);
	driver(std::wstring, bool print_transformed = false);
	bool pfp();

	matrix getbdd(size_t t) const;
	matrix getbdd_one(size_t t) const;
	matrix getdb() const;
	std::wostream& print_term(std::wostream& os, const term& t) const;
	std::wostream& printbdd(std::wostream& os, const matrix& t) const;
	std::wostream& printbdd(std::wostream& os, const matrices& t) const;
	std::wostream& printbdd(std::wostream& os, size_t t, ints ar, int_t rel)
		const;
	std::wostream& printbdd_one(std::wostream& os, size_t t, ints ar,
		int_t rel) const;
	std::wostream& printdb(std::wostream& os, lp *p) const;
	std::wostream& printndb(std::wostream& os, lp *p) const;
	~driver() { if (prog) delete prog; for (cws w:strs_extra)free((wstr)w);}
};

#ifdef DEBUG
extern driver* drv;
std::wostream& printdb(std::wostream& os, lp *p);
std::wostream& printndb(std::wostream& os, lp *p);
std::wostream& printbdd(std::wostream& os, size_t t, ints ar, int_t rel);
std::wostream& printbdd_one(std::wostream& os, size_t t, ints ar, int_t rel);
//std::wostream& printbdd(std::wostream& os, size_t t, size_t bits, ints ar,
//	int_t rel);
#endif
