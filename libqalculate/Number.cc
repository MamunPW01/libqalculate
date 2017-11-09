/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Number.h"
#include "Calculator.h"
#include "Function.h"

#include <limits.h>
#include <sstream>
#include <string.h>
#include "util.h"

#define BIT_PRECISION (((PRECISION) * 3.3219281) + 100)
#define PRECISION_TO_BITS(p) (((p) * 3.3219281) + 100)
#define BITS_TO_PRECISION(p) (::ceil(((p) - 100) / 3.3219281))

#define TO_BIT_PRECISION(p) ((::ceil((p) * 3.3219281)))
#define FROM_BIT_PRECISION(p) ((::floor((p) / 3.3219281)))

#define PRINT_MPFR(x, y) mpfr_out_str(stdout, y, 0, x, MPFR_RNDU); cout << endl;

gmp_randstate_t randstate;

Number nr_e;

string format_number_string(string cl_str, int base, BaseDisplay base_display, bool show_neg, bool format_base_two = true) {
	if(format_base_two && base == 2 && base_display != BASE_DISPLAY_NONE) {
		int i2 = cl_str.length() % 4;
		if(i2 != 0) i2 = 4 - i2;
		if(base_display == BASE_DISPLAY_NORMAL) {
			for(int i = (int) cl_str.length() - 4; i > 0; i -= 4) {
				cl_str.insert(i, 1, ' ');
			}
		}
		for(; i2 > 0; i2--) {
			cl_str.insert(cl_str.begin(), 1, '0');
		}
	}	
	string str = "";
	if(show_neg) {
		str += '-';
	}
	if(base_display == BASE_DISPLAY_NORMAL) {
		if(base == 16) {
			str += "0x";
		} else if(base == 8) {
			str += "0";
		}
	} else if(base_display == BASE_DISPLAY_ALTERNATIVE) {
		if(base == 16) {
			str += "0x0";
		} else if(base == 8) {
			str += "0";
		} else if(base == 2) {
			str += "0b00";
		} 
	}
	str += cl_str;
	return str;
}

string printMPZ(mpz_ptr integ_pre, int base = 10, bool display_sign = true, BaseDisplay base_display = BASE_DISPLAY_NORMAL, bool lower_case = false, bool use_unicode = false) {
	int sign = mpz_sgn(integ_pre);
	if(base == BASE_ROMAN_NUMERALS) {
		if(sign != 0 && mpz_cmpabs_ui(integ_pre, 10000) == -1) {
			string str;
			int value = (int) mpz_get_si(integ_pre);
			if(value < 0) {
				value = -value;
				if(display_sign) {
					str += "-";
				}
			}
			int times = value / 1000;
			for(; times > 0; times--) {
				if(lower_case) str += "m";
				else str += "M";
			}
			value = value % 1000;
			times = value / 100;
			if(times == 9) {
				if(lower_case) str += "c";
				else str += "C";
				if(lower_case) str += "m";
				else str += "M";
				times = 0;
			} else if(times >= 5) {
				if(lower_case) str += "d";
				else str += "D";
				times -= 5;
			} else if(times == 4) {
				times = 0;
				if(lower_case) str += "c";
				else str += "C";
				if(lower_case) str += "d";
				else str += "D";
			}
			for(; times > 0; times--) {
				if(lower_case) str += "c";
				else str += "C";
			}
			value = value % 100;
			times = value / 10;
			if(times == 9) {
				if(lower_case) str += "x";
				else str += "X";
				if(lower_case) str += "c";
				else str += "C";
				times = 0;
			} else if(times >= 5) {
				if(lower_case) str += "l";
				else str += "L";
				times -= 5;
			} else if(times == 4) {
				times = 0;
				if(lower_case) str += "x";
				else str += "X";
				if(lower_case) str += "l";
				else str += "L";
			}
			for(; times > 0; times--) {
				if(lower_case) str += "x";
				else str += "X";
			}
			value = value % 10;
			times = value;
			if(times == 9) {
				if(lower_case) str += "i";
				else str += "I";
				if(lower_case) str += "x";
				else str += "X";
				times = 0;
			} else if(times >= 5) {
				if(lower_case) str += "v";
				else str += "V";
				times -= 5;
			} else if(times == 4) {
				times = 0;
				if(lower_case) str += "i";
				else str += "I";
				if(lower_case) str += "v";
				else str += "V";
			}
			for(; times > 0; times--) {
				if(lower_case) str += "i";
				else str += "I";
			}
			return str;
		} else if(sign != 0) {
			CALCULATOR->error(false, _("Cannot display numbers greater than 9999 or less than -9999 as roman numerals."), NULL);
		}
		base = 10;
	}
	
	mpz_t integ;
	mpz_init_set(integ, integ_pre);
	if(sign == -1) {
		mpz_neg(integ, integ);
	}
	
	string cl_str;
	
	char *tmp = mpz_get_str(NULL, base, integ); 
	cl_str = tmp;
	void (*freefunc)(void *, size_t);
	mp_get_memory_functions (NULL, NULL, &freefunc);
	freefunc(tmp, strlen(tmp) + 1);
	
	if(base == BASE_DUODECIMAL) {
		for(size_t i = 0; i < cl_str.length(); i++) {
			if(cl_str[i] == 'A' || cl_str[i] == 'a') {
				if(use_unicode) {cl_str.replace(i, 1, "↊"); i += strlen("↊") - 1;}
				else cl_str[i] = 'X';
			} else if(cl_str[i] == 'B' || cl_str[i] == 'b') {
				if(use_unicode) {cl_str.replace(i, 1, "↋"); i += strlen("↋") - 1;}
				else cl_str[i] = 'E';
			}
		}
	} else if(base > 10) {
		if(lower_case) {
			for(size_t i = 0; i < cl_str.length(); i++) {
				if(cl_str[i] >= 'A' && cl_str[i] <= 'Z') {
					cl_str[i] += 32;
				}
			}
		} else {
			for(size_t i = 0; i < cl_str.length(); i++) {
				if(cl_str[i] >= 'a' && cl_str[i] <= 'z') {
					cl_str[i] -= 32;
				}
			}
		}
	}
	if(cl_str[cl_str.length() - 1] == '.') {
		cl_str.erase(cl_str.length() - 1, 1);
	}
	
	mpz_clear(integ);
	
	return format_number_string(cl_str, base, base_display, sign == -1 && display_sign);
}
string printMPZ(mpz_srcptr integ_pre, int base = 10, bool display_sign = true, BaseDisplay base_display = BASE_DISPLAY_NORMAL, bool lower_case = false) {
	mpz_t integ;
	mpz_init_set(integ, integ_pre);
	string str = printMPZ(integ, base, display_sign, base_display, lower_case);
	mpz_clear(integ);
	return str;
}

Number::Number() {
	b_imag = false;
	i_value = NULL;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_init(r_value);
	clear();
}
Number::Number(string number, const ParseOptions &po) {
	b_imag = false;
	i_value = NULL;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_init(r_value);
	set(number, po);
}
Number::Number(long int numerator, long int denominator, long int exp_10) {
	b_imag = false;
	i_value = NULL;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_init(r_value);
	set(numerator, denominator, exp_10);
}
Number::Number(const Number &o) {
	b_imag = false;
	i_value = NULL;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_init(r_value);
	set(o);
}
Number::~Number() {
	mpq_clear(r_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	if(i_value) delete i_value;
}

void Number::set(string number, const ParseOptions &po) {

	if(po.base == BASE_ROMAN_NUMERALS) {
		remove_blanks(number);
		Number nr;
		Number cur;
		bool large = false;
		vector<Number> numbers;
		bool capital = false;
		for(size_t i = 0; i < number.length(); i++) {
			switch(number[i]) {
				case 'I': {
					if(!capital && i == number.length() - 1) {
						cur.set(2);
						CALCULATOR->error(false, _("Assuming the unusual practice of letting a last capital I mean 2 in a roman numeral."), NULL);
						break;
					}
				}
				case 'J': {capital = true;}
				case 'i': {}
				case 'j': {
					cur.set(1);
					break;
				}
				case 'V': {capital = true;}
				case 'v': {
					cur.set(5);
					break;
				}
				case 'X': {capital = true;}
				case 'x': {
					cur.set(10);
					break;
				}
				case 'L': {capital = true;}
				case 'l': {
					cur.set(50);
					break;
				}
				case 'C': {capital = true;}
				case 'c': {
					cur.set(100);
					break;
				}
				case 'D': {capital = true;}
				case 'd': {
					cur.set(500);
					break;
				}
				case 'M': {capital = true;}
				case 'm': {
					cur.set(1000);
					break;
				}
				case '(': {
					int multi = 1, multi2 = 0;
					bool turn = false;
					bool error = false;
					i++;
					for(; i < number.length(); i++) {
						if(number[i] == '|') {
							if(!turn) {
								turn = true;
								multi2 = multi;
							} else {
								error = true;
								break;
							}
						} else if(number[i] == ')') {
							if(turn) {
								multi2--;
								if(multi2 < 1) {
									break;
								}	
							} else {
								error = true;
								break;
							}
						} else if(number[i] == '(') {
							if(!turn) {
								multi++;	
							} else {
								error = true;
								break;
							}
						} else {
							error = true;
							i--;
							break;
						}
					}
					if(error | !turn) {
						CALCULATOR->error(true, _("Error in roman numerals: %s."), number.c_str(), NULL);
					} else {
						cur.set(10);
						cur.raise(multi);
						cur.multiply(100);
					}
					break;
				}
				case '|': {
					if(large) {
						cur.clear();
						large = false;
						break;
					} else if(number.length() > i + 1 && number[i + 2] == ')') {
						i++;
						int multi = 1;
						for(; i < number.length(); i++) {
							if(number[i] != ')') {
								i--;
								break;
							}
							multi++;
						}
						cur.set(10);
						cur.raise(multi);
						cur.multiply(50);
						break;
					} else if(number.length() > i + 2 && number[i + 2] == '|') {
						cur.clear();
						large = true;
						break;
					}
				}
				default: {
					cur.clear();
					CALCULATOR->error(true, _("Unknown roman numeral: %c."), number[i], NULL);
				}
			}
			if(!cur.isZero()) {
				if(large) {
					cur.multiply(100000L);
				}
				numbers.resize(numbers.size() + 1);
				numbers[numbers.size() - 1].set(cur);
			}
		}
		vector<Number> values;
		values.resize(numbers.size());
		bool error = false;
		int rep = 1;
		for(size_t i = 0; i < numbers.size(); i++) {
			if(i == 0 || numbers[i].isLessThanOrEqualTo(numbers[i - 1])) {
				nr.add(numbers[i]);
				if(i > 0 && numbers[i].equals(numbers[i - 1])) {
					rep++;
					if(rep > 3 && numbers[i].isLessThan(1000)) {
						error = true;
					} else if(rep > 1 && (numbers[i].equals(5) || numbers[i].equals(50) || numbers[i].equals(500))) {
						error = true;
					}
				} else {
					rep = 1;
				}
			} else {	
				numbers[i - 1].multiply(10);
				if(numbers[i - 1].isLessThan(numbers[i])) {
					error = true;
				}
				numbers[i - 1].divide(10);
				for(int i2 = i - 2; ; i2--) {
					if(i2 < 0) {
						nr.negate();
						nr.add(numbers[i]);
						break;
					} else if(numbers[i2].isGreaterThan(numbers[i2 + 1])) {
						Number nr2(nr);
						nr2.subtract(values[i2]);
						nr.subtract(nr2);
						nr.subtract(nr2);
						nr.add(numbers[i]);
						if(numbers[i2].isLessThan(numbers[i])) {
							error = true;
						}
						break;
					}
					error = true;
				}
			}
			values[i].set(nr);
		}
		if(error) {
			PrintOptions pro;
			pro.base = BASE_ROMAN_NUMERALS;
			CALCULATOR->error(false, _("Errors in roman numerals: \"%s\". Interpreted as %s, which should be written as %s."), number.c_str(), nr.print().c_str(), nr.print(pro).c_str(), NULL);
		}
		values.clear();
		numbers.clear();
		set(nr);
		return;
	}
	mpz_t num, den;
	mpz_init(num);
	mpz_init_set_ui(den, 1);
	int base = po.base;
	remove_blank_ends(number);
	if(base == 16 && number.length() >= 2 && number[0] == '0' && (number[1] == 'x' || number[1] == 'X')) {
		number = number.substr(2, number.length() - 2);
	} else if(base == 8 && number.length() >= 2 && number[0] == '0' && (number[1] == 'o' || number[1] == 'O')) {
		number = number.substr(2, number.length() - 2);
	} else if(base == 8 && number.length() > 1 && number[0] == '0' && number[1] != '.') {
		number.erase(number.begin());
	} else if(base == 2 && number.length() >= 2 && number[0] == '0' && (number[1] == 'b' || number[1] == 'B')) {
		number = number.substr(2, number.length() - 2);
	}
	if(base > 36) base = 36;
	if(base < 0) base = 10;
	long int readprec = 0;
	bool numbers_started = false, minus = false, in_decimals = false, b_cplx = false, had_nonzero = false;
	for(size_t index = 0; index < number.size(); index++) {
		if(number[index] >= '0' && ((base >= 10 && number[index] <= '9') || (base < 10 && number[index] < '0' + base))) {
			mpz_mul_si(num, num, base);
			if(number[index] != '0') {
				mpz_add_ui(num, num, (unsigned long int) number[index] - '0');
				if(!had_nonzero) readprec = 0;
				had_nonzero = true;
			}
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			readprec++;
			numbers_started = true;
		} else if(base == BASE_DUODECIMAL && (number[index] == 'X' || number[index] == 'E' || number[index] == 'x' || number[index] == 'e')) {
			mpz_mul_si(num, num, base);
			mpz_add_ui(num, num, (number[index] == 'E' || number[index] == 'e') ? 11L : 10L);
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			if(!had_nonzero) readprec = 0;
			had_nonzero = true;
			readprec++;
			numbers_started = true;
		} else if(base > 10 && number[index] >= 'a' && number[index] < 'a' + base - 10) {
			mpz_mul_si(num, num, base);
			mpz_add_ui(num, num, (unsigned long int) number[index] - 'a' + 10);
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			if(!had_nonzero) readprec = 0;
			had_nonzero = true;
			readprec++;
			numbers_started = true;
		} else if(base > 10 && number[index] >= 'A' && number[index] < 'A' + base - 10) {
			mpz_mul_si(num, num, base);
			mpz_add_ui(num, num, (unsigned long int) number[index] - 'A' + 10);
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			if(!had_nonzero) readprec = 0;
			had_nonzero = true;
			readprec++;
			numbers_started = true;
		} else if(number[index] == 'E' && base <= 10) {
			index++;
			numbers_started = false;
			bool exp_minus = false;
			unsigned long int exp = 0;
			unsigned long int max_exp = ULONG_MAX / 10;
			while(index < number.size()) {
				if(number[index] >= '0' && number[index] <= '9') {
					if(exp > max_exp) {
						CALCULATOR->error(true, _("Too large exponent."), NULL);
					} else {
						exp = exp * 10;
						exp = exp + number[index] - '0';
						numbers_started = true;
					}
				} else if(!numbers_started && number[index] == '-') {
					exp_minus = !exp_minus;
				}
				index++;
			}
			if(exp_minus) {
				mpz_t e_den;
				mpz_init(e_den);
				mpz_ui_pow_ui(e_den, 10, exp);
				mpz_mul(den, den, e_den);
				mpz_clear(e_den);
			} else {
				mpz_t e_num;
				mpz_init(e_num);
				mpz_ui_pow_ui(e_num, 10, exp);
				mpz_mul(num, num, e_num);
				mpz_clear(e_num);
			}
			break;
		} else if(number[index] == '.') {
			in_decimals = true;
		} else if(number[index] == ':') {
			if(in_decimals) {
				CALCULATOR->error(true, _("\':\' in decimal number ignored (decimal point detected)."), NULL);
			} else {
				size_t index_colon = index;
				Number divisor(1, 1);
				Number num_temp;
				clear();
				i_precision = -1;				
				index = 0;				
				while(index_colon < number.size()) {
					num_temp.set(number.substr(index, index_colon - index), po);
					if(!num_temp.isZero()) {
						num_temp.divide(divisor);						
						add(num_temp);
					}
					index = index_colon + 1;
					index_colon = number.find(":", index);
					divisor.multiply(Number(60, 1));
				}
				num_temp.set(number.substr(index), po);
				if(!num_temp.isZero()) {
					num_temp.divide(divisor);
					add(num_temp);
				}
				return;
			}
		} else if(!numbers_started && number[index] == '-') {
			minus = !minus;
		} else if(number[index] == 'i') {
			b_cplx = true;
		} else if(number[index] != ' ') {
			CALCULATOR->error(true, _("Character \'%c\' was ignored in the number \"%s\" with base %s."), number[index], number.c_str(), i2s(base).c_str(), NULL);
		}
	}
	clear();
	if((po.read_precision == ALWAYS_READ_PRECISION || (in_decimals && po.read_precision == READ_PRECISION_WHEN_DECIMALS)) && CALCULATOR->usesIntervalArithmetics()) {
		mpz_mul_si(num, num, 2);
		mpz_mul_si(den, den, 2);
		
		mpq_t rv1, rv2;
		mpq_inits(rv1, rv2, NULL);
		
		mpz_add_ui(num, num, 1);
		if(minus) mpz_neg(mpq_numref(rv1), num);
		else mpz_set(mpq_numref(rv1), num);
		mpz_set(mpq_denref(rv1), den);
		mpq_canonicalize(rv1);
		
		mpz_sub_ui(num, num, 2);
		if(minus) mpz_neg(mpq_numref(rv2), num);
		else mpz_set(mpq_numref(rv2), num);
		mpz_set(mpq_denref(rv2), den);
		mpq_canonicalize(rv2);

		mpfr_init2(fu_value, BIT_PRECISION);
		mpfr_init2(fl_value, BIT_PRECISION);
		mpfr_clear_flags();
			
		mpfr_set_q(fu_value, minus ? rv2 : rv1, MPFR_RNDD);
		mpfr_set_q(fl_value, minus ? rv1 : rv2, MPFR_RNDU);
		
		if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);

		n_type = NUMBER_TYPE_FLOAT;
		
		testErrors(2);
		
		if(b_cplx) {
			if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
			i_value->set(*this, false, true);
			clearReal();
		}
		
		mpq_clears(rv1, rv2, NULL);
	} else {
		if(minus) mpz_neg(num, num);
		if(b_cplx) {
			if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
			i_value->setInternal(num, den, false, true);
			mpq_canonicalize(i_value->internalRational());
		} else {
			mpz_set(mpq_numref(r_value), num);
			mpz_set(mpq_denref(r_value), den);
			mpq_canonicalize(r_value);
		}
		if(po.read_precision == ALWAYS_READ_PRECISION || (in_decimals && po.read_precision == READ_PRECISION_WHEN_DECIMALS)) {
			if(base != 10) {
				Number precmax(10);
				precmax.raise(readprec);
				precmax--;
				precmax.log(base);
				precmax.floor();
				readprec = precmax.intValue();
			}
			if(b_cplx) i_value->setPrecision(readprec);
			setPrecision(readprec);
		}
	}
	mpz_clears(num, den, NULL);
}
void Number::set(long int numerator, long int denominator, long int exp_10, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	mpq_set_si(r_value, numerator, denominator == 0 ? 1 : denominator);
	mpq_canonicalize(r_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(exp_10 != 0) {
		exp10(exp_10);
	}
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setFloat(double d_value) {
	b_approx = true;
	if(n_type != NUMBER_TYPE_FLOAT) {mpfr_init2(fu_value, BIT_PRECISION); mpfr_init2(fl_value, BIT_PRECISION);}
	mpfr_set_d(fu_value, d_value, MPFR_RNDU);
	mpfr_set_d(fl_value, d_value, MPFR_RNDD);
	n_type = NUMBER_TYPE_FLOAT;
	mpq_set_ui(r_value, 0, 1);
	if(i_value) i_value->clear();
}
void Number::setInterval(const Number &nr_lower, const Number &nr_upper, bool keep_precision) {

	clear(keep_precision);

	mpfr_init2(fu_value, BIT_PRECISION);
	mpfr_init2(fl_value, BIT_PRECISION);
	
	Number nr_l(nr_lower), nr_u(nr_upper);
	nr_l.setToFloatingPoint();
	nr_u.setToFloatingPoint();
	
	mpfr_clear_flags();

	if(mpfr_cmp(nr_l.internalUpperFloat(), nr_u.internalUpperFloat()) > 0) mpfr_set(fu_value, nr_l.internalUpperFloat(), MPFR_RNDU);
	else mpfr_set(fu_value, nr_u.internalUpperFloat(), MPFR_RNDU);
	if(mpfr_cmp(nr_l.internalLowerFloat(), nr_u.internalLowerFloat()) > 0) mpfr_set(fl_value, nr_u.internalLowerFloat(), MPFR_RNDD);
	else mpfr_set(fl_value, nr_l.internalLowerFloat(), MPFR_RNDD);
	
	setPrecisionAndApproximateFrom(nr_l);
	setPrecisionAndApproximateFrom(nr_u);
	
	n_type = NUMBER_TYPE_FLOAT;

}

void Number::setInternal(mpz_srcptr mpz_value, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	mpq_set_z(r_value, mpz_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setInternal(const mpz_t &mpz_value, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	mpq_set_z(r_value, mpz_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setInternal(const mpq_t &mpq_value, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	mpq_set(r_value, mpq_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setInternal(const mpz_t &mpz_num, const mpz_t &mpz_den, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	mpz_set(mpq_numref(r_value), mpz_num);
	mpz_set(mpq_denref(r_value), mpz_den);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setInternal(const mpfr_t &mpfr_value, bool merge_precision, bool keep_imag) {
	b_approx = true;
	if(n_type != NUMBER_TYPE_FLOAT) {mpfr_init2(fu_value, BIT_PRECISION); mpfr_init2(fl_value, BIT_PRECISION);}
	mpfr_set(fu_value, mpfr_value, MPFR_RNDU);
	mpfr_set(fl_value, mpfr_value, MPFR_RNDD);
	n_type = NUMBER_TYPE_FLOAT;
	mpq_set_ui(r_value, 0, 1);
	if(!keep_imag && i_value) i_value->clear();
	if(!merge_precision && (i_precision < 0 || i_precision > PRECISION)) {
		i_precision = PRECISION;
		if(i_value) setPrecisionAndApproximateFrom(*i_value);
	}
}

void Number::setImaginaryPart(const Number &o) {
	if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
	i_value->set(o, false, true);
	setPrecisionAndApproximateFrom(*i_value);
}
void Number::setImaginaryPart(long int numerator, long int denominator, long int exp_10) {
	if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
	i_value->set(numerator, denominator, exp_10);
}
void Number::set(const Number &o, bool merge_precision, bool keep_imag) {
	mpq_set(r_value, o.internalRational());
	if(o.internalType() == NUMBER_TYPE_FLOAT) {
		if(n_type != NUMBER_TYPE_FLOAT) {mpfr_init2(fu_value, BIT_PRECISION); mpfr_init2(fl_value, BIT_PRECISION);}
		mpfr_set(fu_value, o.internalUpperFloat(), MPFR_RNDU);
		mpfr_set(fl_value, o.internalLowerFloat(), MPFR_RNDD);
	}
	n_type = o.internalType();
	if(!merge_precision) {
		i_precision = -1;
		b_approx = false;
	}
	if(o.isApproximate()) b_approx = true;
	if(i_precision < 0 || o.precision() < i_precision) i_precision = o.precision();
	if(!keep_imag && !b_imag) {
		if(o.hasImaginaryPart()) {
			setImaginaryPart(*o.internalImaginary());
		} else if(i_value) {
			i_value->clear();
		}
	}
}
void Number::setInfinity(bool keep_precision) {
	clear(keep_precision);
	n_type = NUMBER_TYPE_INFINITY;
}
void Number::setPlusInfinity(bool keep_precision) {
	clear(keep_precision);
	n_type = NUMBER_TYPE_PLUS_INFINITY;
}
void Number::setMinusInfinity(bool keep_precision) {
	clear(keep_precision);
	n_type = NUMBER_TYPE_MINUS_INFINITY;
}

void Number::clear(bool keep_precision) {
	if(i_value) i_value->clear();
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	if(n_type == NUMBER_TYPE_FLOAT) {
		mpfr_clear(fl_value);
		mpfr_clear(fu_value);
	}
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_set_si(r_value, 0, 1);
}
void Number::clearReal() {
	if(n_type == NUMBER_TYPE_FLOAT) {
		mpfr_clear(fl_value);
		mpfr_clear(fu_value);
	}
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_set_si(r_value, 0, 1);
}
void Number::clearImaginary() {
	if(i_value) i_value->clear();
}

const mpq_t &Number::internalRational() const {
	return r_value;
}
const mpfr_t &Number::internalUpperFloat() const {
	return fu_value;
}
const mpfr_t &Number::internalLowerFloat() const {
	return fl_value;
}
mpq_t &Number::internalRational() {
	return r_value;
}
mpfr_t &Number::internalUpperFloat() {
	return fu_value;
}
mpfr_t &Number::internalLowerFloat() {
	return fl_value;
}
Number *Number::internalImaginary() const {
	return i_value;
}
void Number::markAsImaginaryPart(bool is_imag) {
	b_imag = is_imag;
}
const NumberType &Number::internalType() const {
	return n_type;
}
bool Number::setToFloatingPoint() {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpfr_init2(fu_value, BIT_PRECISION);
		mpfr_init2(fl_value, BIT_PRECISION);
		
		mpfr_clear_flags();

		if(!CALCULATOR->usesIntervalArithmetics()) {
			mpfr_set_q(fl_value, r_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_set_q(fu_value, r_value, MPFR_RNDU);
			mpfr_set_q(fl_value, r_value, MPFR_RNDD);
		}

		if(!testFloatResult(false, 1, false)) {
			mpfr_clears(fu_value, fl_value, NULL);
			return false;
		}
		mpq_set_ui(r_value, 0, 1);
		n_type = NUMBER_TYPE_FLOAT;
	}
	return true;
}
void Number::precisionToInterval() {
	if(i_precision >= 0 && isReal() && !isInterval()) {
		if(!setToFloatingPoint()) return;
		mpfr_t f_log;
		mpfr_init2(f_log, mpfr_get_prec(fl_value));
		mpfr_log10(f_log, fu_value, MPFR_RNDN);
		mpfr_floor(f_log, f_log);
		mpfr_sub_ui(f_log, f_log, i_precision, MPFR_RNDN);
		mpfr_ui_pow(f_log, 10, f_log, MPFR_RNDD);
		mpfr_div_ui(f_log, f_log, 2, MPFR_RNDD);
		mpfr_sub(fl_value, fl_value, f_log, MPFR_RNDU);
		mpfr_add(fu_value, fu_value, f_log, MPFR_RNDD);
		mpfr_clear(f_log);
	}
}
bool Number::intervalToPrecision() {
	if(n_type == NUMBER_TYPE_FLOAT && !mpfr_equal_p(fl_value, fu_value)) {
		mpfr_t f_diff, f_mid;
		mpfr_inits2(mpfr_get_prec(fl_value), f_diff, f_mid, NULL);
		mpfr_sub(f_diff, fu_value, fl_value, MPFR_RNDN);
		mpfr_div_ui(f_diff, f_diff, 2, MPFR_RNDN);
		mpfr_add(f_mid, fl_value, f_diff, MPFR_RNDN);
		mpfr_mul_ui(f_diff, f_diff, 2, MPFR_RNDN);
		mpfr_div(f_diff, f_mid, f_diff, MPFR_RNDU);
		mpfr_log10(f_diff, f_diff, MPFR_RNDD);
		long int i_prec = mpfr_get_si(f_diff, MPFR_RNDD);
		if(i_prec < 0) i_prec = 0;
		if(i_precision < 0 || i_prec < i_precision) i_precision = i_prec;
		mpfr_clears(f_diff, f_mid, NULL);
		b_approx = true;
	}
	return true;
}
void Number::setUncertainty(const Number &o) {
	if(!o.isReal() || !isReal()) return;
	b_approx = true;
	if(!CALCULATOR->usesIntervalArithmetics()) {
		Number nr(*this);
		nr.divide(o);
		nr.divide(2);
		nr.log(10);
		nr.floor();
		long int i_prec = nr.lintValue();
		if(i_prec > 0) {
			if(i_precision < 0 || i_prec < i_precision) i_precision = i_prec;
			return;
		}
	}
	mpfr_clear_flags();
	if(isRational()) {
		mpfr_inits2(BIT_PRECISION, fl_value, fu_value, NULL);
		if(o.isRational()) {
			mpfr_set_q(fl_value, r_value, MPFR_RNDD);
			mpfr_set_q(fu_value, r_value, MPFR_RNDU);
			mpq_set_ui(r_value, 0, 1);
			n_type = NUMBER_TYPE_FLOAT;
		}
		if(!setToFloatingPoint()) return;
	}
	if(o.isRational()) {
		mpfr_sub_q(fl_value, fl_value, o.internalRational(), MPFR_RNDD);
		mpfr_add_q(fu_value, fu_value, o.internalRational(), MPFR_RNDU);
	} else if(isRational()) {
		mpfr_sub_q(fl_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
		mpfr_neg(fl_value, fl_value, MPFR_RNDD);
		mpfr_add_q(fu_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
		mpq_set_ui(r_value, 0, 1);
		n_type = NUMBER_TYPE_FLOAT;
	} else {
		mpfr_sub(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
		mpfr_add(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
	}
	testErrors(2);
}
Number Number::uncertainty() const {
	if(!isInterval()) return Number();
	mpfr_t f_mid;
	mpfr_init2(f_mid, BIT_PRECISION);
	mpfr_sub(f_mid, fu_value, fl_value, MPFR_RNDU);
	mpfr_div_ui(f_mid, f_mid, 2, MPFR_RNDU);
	Number nr;
	nr.setInternal(f_mid);
	mpfr_clear(f_mid);
	return nr;
}
Number Number::relativeUncertainty() const {
	if(!isInterval()) return Number();
	mpfr_t f_mid, f_diff;
	mpfr_inits2(BIT_PRECISION, f_mid, f_diff, NULL);
	mpfr_sub(f_mid, fu_value, fl_value, MPFR_RNDU);
	mpfr_div_ui(f_diff, f_diff, 2, MPFR_RNDU);
	mpfr_add(f_mid, fl_value, f_diff, MPFR_RNDN);
	mpfr_div(f_mid, f_diff, f_mid, MPFR_RNDN);
	Number nr;
	nr.setInternal(f_mid);
	mpfr_clears(f_mid, f_diff, NULL);
	return nr;
}

double Number::floatValue() const {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		return mpq_get_d(r_value);
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_get_d(fu_value, MPFR_RNDN) / 2.0 + mpfr_get_d(fl_value, MPFR_RNDN) / 2.0;
	} 
	return 0.0;
}
int Number::intValue(bool *overflow) const {
	if(isInfinite()) return 0;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_fits_sint_p(mpq_numref(r_value)) == 0) {
			if(overflow) *overflow = true;
			if(mpz_sgn(mpq_numref(r_value)) == -1) return INT_MIN;
			return INT_MAX;	
		}
		return (int) mpz_get_si(mpq_numref(r_value));
	} else {
		Number nr;
		nr.set(*this, false, true);
		nr.round();
		return nr.intValue(overflow);
	}
}
unsigned int Number::uintValue(bool *overflow) const {
	if(isInfinite()) return 0;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_fits_uint_p(mpq_numref(r_value)) == 0) {
			if(overflow) *overflow = true;
			if(mpz_sgn(mpq_numref(r_value)) == -1) return 0;
			return UINT_MAX;	
		}
		return (unsigned int) mpz_get_ui(mpq_numref(r_value));
	} else {
		Number nr;
		nr.set(*this, false, true);
		nr.round();
		return nr.uintValue(overflow);
	}
}
long int Number::lintValue(bool *overflow) const {
	if(isInfinite()) return 0;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_fits_slong_p(mpq_numref(r_value)) == 0) {
			if(overflow) *overflow = true;
			if(mpz_sgn(mpq_numref(r_value)) == -1) return LONG_MIN;
			return LONG_MAX;	
		}
		return mpz_get_si(mpq_numref(r_value));
	} else {
		Number nr;
		nr.set(*this, false, true);
		nr.round();
		return nr.lintValue(overflow);
	}
}
unsigned long int Number::ulintValue(bool *overflow) const {
	if(isInfinite()) return 0;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_fits_ulong_p(mpq_numref(r_value)) == 0) {
			if(overflow) *overflow = true;
			if(mpz_sgn(mpq_numref(r_value)) == -1) return 0;
			return ULONG_MAX;
		}
		return mpz_get_ui(mpq_numref(r_value));
	} else {
		Number nr;
		nr.set(*this, false, true);
		nr.round();
		return nr.ulintValue(overflow);
	}
}

bool Number::isApproximate() const {
	return b_approx;
}
bool Number::isFloatingPoint() const {
	return (n_type == NUMBER_TYPE_FLOAT);
}
bool Number::isInterval() const {
	return n_type == NUMBER_TYPE_FLOAT && !mpfr_equal_p(fl_value, fu_value);
}
void Number::setApproximate(bool is_approximate) {
	if(is_approximate != isApproximate()) {
		if(is_approximate) {
			i_precision = PRECISION;
			b_approx = true;
		} else {
			i_precision = -1;
			b_approx = false;
		}
	}
}

int Number::precision() const {
	return i_precision;
}
void Number::setPrecision(int prec) {
	i_precision = prec;
	if(i_precision >= 0) b_approx = true;
}

bool Number::isUndefined() const {
	return false;
}
bool Number::isInfinite() const {
	return n_type >= NUMBER_TYPE_INFINITY;
}
bool Number::isInfinity() const {
	return n_type == NUMBER_TYPE_INFINITY;
}
bool Number::isPlusInfinity() const {
	return n_type == NUMBER_TYPE_PLUS_INFINITY;
}
bool Number::isMinusInfinity() const {
	return n_type == NUMBER_TYPE_MINUS_INFINITY;
}

Number Number::realPart() const {
	Number real_part;
	real_part.set(*this, true, true);
	return real_part;
}
Number Number::imaginaryPart() const {
	if(isInfinite()) return *this;
	if(!i_value) return Number();
	return *i_value;
}
Number Number::numerator() const {
	Number num;
	num.setInternal(mpq_numref(r_value));
	return num;
}
Number Number::denominator() const {
	Number den;
	den.setInternal(mpq_denref(r_value));
	return den;
}
Number Number::complexNumerator() const {
	Number num;
	if(hasImaginaryPart()) num.setInternal(mpq_numref(i_value->internalRational()));
	return num;
}
Number Number::complexDenominator() const {
	Number den(1, 0);
	if(hasImaginaryPart()) den.setInternal(mpq_denref(i_value->internalRational()));
	return den;
}

void Number::operator = (const Number &o) {set(o);}
void Number::operator = (long int i) {set(i, 1);}
void Number::operator -- (int) {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpz_sub(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_sub_ui(fl_value, fl_value, 1, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_sub_ui(fu_value, fu_value, 1, MPFR_RNDU);
			mpfr_sub_ui(fl_value, fl_value, 1, MPFR_RNDD);
		}
	}
}
void Number::operator ++ (int) {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpz_add(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_add_ui(fl_value, fl_value, 1, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_add_ui(fu_value, fu_value, 1, MPFR_RNDU);
			mpfr_add_ui(fl_value, fl_value, 1, MPFR_RNDD);
		}
	}
}
Number Number::operator - () const {Number o(*this); o.negate(); return o;}
Number Number::operator * (const Number &o) const {Number o2(*this); o2.multiply(o); return o2;}
Number Number::operator / (const Number &o) const {Number o2(*this); o2.divide(o); return o2;}
Number Number::operator + (const Number &o) const {Number o2(*this); o2.add(o); return o2;}
Number Number::operator - (const Number &o) const {Number o2(*this); o2.subtract(o); return o2;}
Number Number::operator ^ (const Number &o) const {Number o2(*this); o2.raise(o); return o2;}
Number Number::operator * (long int i) const {Number o2(*this); o2.multiply(i); return o2;}
Number Number::operator / (long int i) const {Number o2(*this); o2.divide(i); return o2;}
Number Number::operator + (long int i) const {Number o2(*this); o2.add(i); return o2;}
Number Number::operator - (long int i) const {Number o2(*this); o2.subtract(i); return o2;}
Number Number::operator ^ (long int i) const {Number o2(*this); o2.raise(i); return o2;}
Number Number::operator && (const Number &o) const {Number o2(*this); o2.add(o, OPERATION_LOGICAL_AND); return o2;}
Number Number::operator || (const Number &o) const {Number o2(*this); o2.add(o, OPERATION_LOGICAL_OR); return o2;}
Number Number::operator ! () const {Number o(*this); o.setLogicalNot(); return o;}
		
void Number::operator *= (const Number &o) {multiply(o);}
void Number::operator /= (const Number &o) {divide(o);}
void Number::operator += (const Number &o) {add(o);}
void Number::operator -= (const Number &o) {subtract(o);}
void Number::operator ^= (const Number &o) {raise(o);}
void Number::operator *= (long int i) {multiply(i);}
void Number::operator /= (long int i) {divide(i);}
void Number::operator += (long int i) {add(i);}
void Number::operator -= (long int i) {subtract(i);}
void Number::operator ^= (long int i) {raise(i);}
	
bool Number::operator == (const Number &o) const {return equals(o);}
bool Number::operator != (const Number &o) const {return !equals(o);}
bool Number::operator == (long int i) const {return equals(i);}
bool Number::operator != (long int i) const {return !equals(i);}

bool Number::bitAnd(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	mpz_and(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::bitOr(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	mpz_ior(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::bitXor(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	mpz_xor(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::bitNot() {
	if(!isInteger()) return false;
	mpz_com(mpq_numref(r_value), mpq_numref(r_value));
	return true;
}
bool Number::bitEqv(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	bitXor(o);
	bitNot();
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shiftLeft(const Number &o) {
	if(!o.isInteger() || !isInteger() || o.isNegative()) return false;
	bool overflow = false;
	long int y = o.lintValue(&overflow);
	if(overflow) return false;
	mpz_mul_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) y);
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shiftRight(const Number &o) {
	if(!o.isInteger() || !isInteger() || o.isNegative()) return false;
	bool overflow = false;
	long int y = o.lintValue(&overflow);
	if(overflow) return false;
	mpz_tdiv_q_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) y);
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shift(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	bool overflow = false;
	long int y = o.lintValue(&overflow);
	if(overflow) return false;
	if(y < 0) mpz_tdiv_q_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) -y);
	else mpz_mul_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) y);
	setPrecisionAndApproximateFrom(o);
	return true;
}

bool Number::hasRealPart() const {
	if(isInfinite()) return true;
	if(n_type == NUMBER_TYPE_RATIONAL) return mpq_sgn(r_value) != 0;
	return !mpfr_zero_p(fu_value) || !mpfr_zero_p(fl_value);
}
bool Number::hasImaginaryPart() const {
	return i_value && !i_value->isZero();
}
bool Number::testErrors(int error_level) const {
	if(mpfr_underflow_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point underflow"), NULL); return true;}
	if(mpfr_overflow_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point overflow"), NULL); return true;}
	if(mpfr_divby0_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point division by zero exception"), NULL); return true;}
	if(mpfr_nanflag_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point not a number exception"), NULL); return true;}
	if(mpfr_erangeflag_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point range exception"), NULL); return true;}
	return false;
}
bool testComplex(Number *this_nr, Number *i_nr) {
	if(i_nr && !i_nr->isZero() && i_nr->isFloatingPoint()) {
		Number test1;
		test1.set(*this_nr, false, true);
		Number test2(test1);
		test2.add(*i_nr);
		if(test1 == test2) {
			i_nr->clear(true);
			return true;
		}
	}
	return false;
}
bool Number::testFloatResult(bool allow_infinite_result, int error_level, bool test_integer) {
	if(mpfr_underflow_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point underflow"), NULL); return false;}
	if(mpfr_overflow_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point overflow"), NULL); return false;}
	if(mpfr_divby0_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point division by zero exception"), NULL); return false;}
	if(mpfr_erangeflag_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point range exception"), NULL); return false;}
	if(mpfr_nan_p(fu_value) || mpfr_nan_p(fl_value)) return false;
	if(mpfr_nanflag_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point not a number exception"), NULL); return false;}
	if(mpfr_inexflag_p()) {
		b_approx = true;
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval() && (i_precision < 0 || i_precision > FROM_BIT_PRECISION(BIT_PRECISION))) i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
	}
	mpfr_clear_flags();
	if(mpfr_inf_p(fl_value) && mpfr_inf_p(fu_value) && mpfr_sgn(fl_value) == mpfr_sgn(fu_value)) {
		if(b_imag || !allow_infinite_result) return false;
		int sign = mpfr_sgn(fl_value);
		if(sign > 0) n_type = NUMBER_TYPE_PLUS_INFINITY;
		else if(sign < 0) n_type = NUMBER_TYPE_MINUS_INFINITY;
		else n_type = NUMBER_TYPE_INFINITY;
		mpfr_clears(fl_value, fu_value, NULL);
	} else if(mpfr_inf_p(fu_value) || mpfr_inf_p(fu_value)) {
		return false;
	}
	if(test_integer) testInteger();
	if(!b_imag) testComplex(this, i_value);
	return true;
}
void Number::testInteger() {
	if(isFloatingPoint() && !isInfinite()) {
		if(mpfr_equal_p(fu_value, fl_value) && mpfr_integer_p(fl_value) && mpfr_integer_p(fu_value)) {
			mpfr_get_z(mpq_numref(r_value), fl_value, MPFR_RNDN);
			mpfr_clears(fl_value, fu_value, NULL);
			n_type = NUMBER_TYPE_RATIONAL;
		}
	}
	if(i_value) i_value->testInteger();
}
void Number::setPrecisionAndApproximateFrom(const Number &o) {
	if(o.precision() >= 0 && (i_precision < 0 || o.precision() < i_precision)) i_precision = o.precision();
	if(o.isApproximate()) b_approx = true;
}

bool Number::isComplex() const {
	return i_value && !i_value->isZero();
}
Number Number::integer() const {
	if(isInteger()) return *this;
	Number nr(*this);
	nr.round();
	return nr;
}
bool Number::isInteger(IntegerType integer_type) const {
	if(isInfinite()) return false;
	if(isComplex()) return false;
	if(isFloatingPoint()) return false;
	if(mpz_cmp_ui(mpq_denref(r_value), 1) != 0) return false;
	switch(integer_type) {
		case INTEGER_TYPE_NONE: {return true;}
		case INTEGER_TYPE_SIZE: {}
		case INTEGER_TYPE_UINT: {return mpz_fits_uint_p(mpq_numref(r_value)) != 0;}
		case INTEGER_TYPE_SINT: {return mpz_fits_sint_p(mpq_numref(r_value)) != 0;}
		case INTEGER_TYPE_ULONG: {return mpz_fits_ulong_p(mpq_numref(r_value)) != 0;}
		case INTEGER_TYPE_SLONG: {return mpz_fits_slong_p(mpq_numref(r_value)) != 0;}
	}
	return true;
}
bool Number::isRational() const {
	return !isFloatingPoint() && !isInfinite() && !isComplex();
}
bool Number::isReal() const {
	return !isInfinite() && !isComplex();
}
bool Number::isFraction() const {
	if(isInfinite()) return false;
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) return mpz_cmpabs(mpq_denref(r_value), mpq_numref(r_value)) > 0;
	bool frac_u = mpfr_cmp_ui(fu_value, 1) < 0 && mpfr_cmp_si(fu_value, -1) > 0;
	bool frac_l = mpfr_cmp_ui(fl_value, 1) < 0 && mpfr_cmp_si(fl_value, -1) > 0;
	return frac_u && frac_l;
}
bool Number::isZero() const {
	if(i_value && !i_value->isZero()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {
		if(mpfr_zero_p(fu_value) && mpfr_zero_p(fl_value)) return true;
	} else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) == 0;
	return false;
}
bool Number::isNonZero() const {
	if(i_value && i_value->isNonZero()) return true;
	if(n_type == NUMBER_TYPE_FLOAT) return !mpfr_zero_p(fu_value) && mpfr_sgn(fu_value) == mpfr_sgn(fl_value);
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) != 0;
	return true;
}
bool Number::isOne() const {
	if(!isReal()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_cmp_ui(fu_value, 1) == 0 && mpfr_cmp_ui(fl_value, 1) == 0;}
	return mpz_cmp(mpq_denref(r_value), mpq_numref(r_value)) == 0;
}
bool Number::isTwo() const {
	if(!isReal()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_cmp_ui(fu_value, 2) == 0 && mpfr_cmp_ui(fl_value, 2) == 0;}
	return mpq_cmp_si(r_value, 2, 1) == 0;
}
bool Number::isI() const {
	if(isInfinite()) return false;
	if(!i_value || !i_value->isOne()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_zero_p(fu_value) && mpfr_zero_p(fl_value);}
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) == 0;
	return true;
}
bool Number::isMinusOne() const {
	if(!isReal()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_cmp_si(fu_value, -1) == 0 && mpfr_cmp_si(fl_value, -1) == 0;}
	return mpq_cmp_si(r_value, -1, 1) == 0;
}
bool Number::isMinusI() const {
	if(isInfinite()) return false;
	if(!i_value || !i_value->isMinusOne()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_zero_p(fu_value) && mpfr_zero_p(fl_value);
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) == 0;
	return true;
}
bool Number::isNegative() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(fu_value) < 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) < 0;
	else if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	return false;
}
bool Number::isNonNegative() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_sgn(fl_value) >= 0;}
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) >= 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::isPositive() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_sgn(fl_value) > 0;}
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) > 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::isNonPositive() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_sgn(fu_value) <= 0;}
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) <= 0;
	else if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	return false;
}
bool Number::realPartIsNegative() const {
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(fu_value) < 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) < 0;
	else if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	return false;
}
bool Number::realPartIsPositive() const {
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(fl_value) > 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) > 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::realPartIsNonZero() const {
	if(n_type == NUMBER_TYPE_FLOAT) return !mpfr_zero_p(fu_value) && mpfr_sgn(fu_value) == mpfr_sgn(fl_value);
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) != 0;
	return true;
}
bool Number::imaginaryPartIsNegative() const {
	if(isInfinite()) return false;
	return i_value && i_value->isNegative();
}
bool Number::imaginaryPartIsPositive() const {
	if(isInfinite()) return false;
	return i_value && i_value->isPositive();
}
bool Number::imaginaryPartIsNonZero() const {
	return i_value && i_value->isNonZero();
}
bool Number::hasNegativeSign() const {
	if(hasRealPart()) return realPartIsNegative();
	return imaginaryPartIsNegative();
}
bool Number::hasPositiveSign() const {
	if(hasRealPart()) return realPartIsPositive();
	return imaginaryPartIsPositive();
}
bool Number::equalsZero() const {
	return isZero();
}
bool Number::equals(const Number &o) const {
	if(isInfinite() || o.isInfinite()) return false;
	if(o.isInfinite()) return false;
	if(o.isComplex()) {
		if(!i_value || !i_value->equals(*o.internalImaginary())) return false;
	} else if(isComplex()) {
		return false;
	}
	if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_q(o.internalLowerFloat(), r_value) == 0 && mpfr_cmp_q(o.internalUpperFloat(), r_value) == 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isFloatingPoint()) return mpfr_equal_p(fu_value, fl_value) && mpfr_equal_p(fl_value, o.internalLowerFloat()) && mpfr_equal_p(fu_value, o.internalUpperFloat());
		else return mpfr_cmp_q(fu_value, o.internalRational()) == 0 && mpfr_cmp_q(fl_value, o.internalRational()) == 0;
	}
	return mpq_cmp(r_value, o.internalRational()) == 0;
}
bool Number::equals(long int i) const {
	if(isInfinite()) return false;
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_cmp_si(fl_value, i) == 0 && mpfr_cmp_si(fu_value, i) == 0;}
	return mpq_cmp_si(r_value, i, 1) == 0;
}
int Number::equalsApproximately(const Number &o, int prec) const {
	if(isInfinite() || o.isInfinite()) return false;
	if(equals(o)) return true;
	int b = 1;
	if(o.isComplex()) {
		if(i_value) {
			b = i_value->equalsApproximately(*o.internalImaginary(), prec);
			if(b == 0) return b;
		} else {
			b = o.internalImaginary()->equalsApproximately(nr_zero, prec);
			if(b == 0) return b;
		}
	} else if(isComplex()) {
		b = i_value->equalsApproximately(nr_zero, prec);
		if(b == 0) return b;
	}
	bool prec_choosen = prec >= 0;
	if(prec == EQUALS_PRECISION_LOWEST) {
		prec = PRECISION;
		if(i_precision >= 0 && i_precision < prec) prec = i_precision;
		if(o.precision() >= 0 && o.precision() < prec) prec = o.precision();
	} else if(prec == EQUALS_PRECISION_HIGHEST) {
		prec = i_precision;
		if(o.precision() >= 0 && o.precision() > prec) prec = o.precision();
		if(prec < 0) prec = PRECISION;
	} else if(prec == EQUALS_PRECISION_DEFAULT) {
		prec = PRECISION;
	}
	if(prec_choosen || isApproximate() || o.isApproximate()) {
		mpfr_t test1, test2;
		mpfr_inits2(::ceil(prec * 3.3219281), test1, test2, NULL);
		if(n_type == NUMBER_TYPE_FLOAT) {
			if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
				mpfr_set(test1, fl_value, MPFR_RNDN);
			} else {
				mpfr_t test_c;
				mpfr_init2(test_c, BIT_PRECISION);
				mpfr_sub(test_c, fu_value, fl_value, MPFR_RNDN);
				mpfr_div_ui(test_c, test_c, 2, MPFR_RNDN);
				mpfr_add(test1, fl_value, test_c, MPFR_RNDN);
				mpfr_clear(test_c);
			}
		} else {
			mpfr_set_q(test1, r_value, MPFR_RNDN);
		}
		if(o.isFloatingPoint()) {
			if(!CALCULATOR->usesIntervalArithmetics() && !o.isInterval()) {
				mpfr_set(test2, o.internalLowerFloat(), MPFR_RNDN);
			} else {
				mpfr_t test_c;
				mpfr_init2(test_c, BIT_PRECISION);
				mpfr_sub(test_c, o.internalUpperFloat(), o.internalLowerFloat(), MPFR_RNDN);
				mpfr_div_ui(test_c, test_c, 2, MPFR_RNDN);
				mpfr_add(test2, o.internalLowerFloat(), test_c, MPFR_RNDN);
				mpfr_clear(test_c);
			}
		} else {
			mpfr_set_q(test2, o.internalRational(), MPFR_RNDN);
		}
		bool b2 = mpfr_equal_p(test1, test2);
		if(!b2) {
			b = 0;
		}
		mpfr_clears(test1, test2, NULL);
		return b;
	}
	if(b == 1) b = 0;
	return b;
}
ComparisonResult Number::compare(long int i) const {return compare(Number(i, 1));}
ComparisonResult Number::compare(const Number &o) const {
	if(n_type == NUMBER_TYPE_INFINITY || o.isInfinity()) return COMPARISON_RESULT_UNKNOWN;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) {
		if(o.isPlusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_LESS;
	}
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
		if(o.isMinusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_GREATER;
	}
	if(o.isPlusInfinity()) return COMPARISON_RESULT_GREATER;
	if(o.isMinusInfinity()) return COMPARISON_RESULT_LESS;
	if(equals(o)) return COMPARISON_RESULT_EQUAL;
	if(!isComplex() && !o.isComplex()) {
		int i = 0, i2 = 0;
		if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
			i = mpfr_cmp_q(o.internalLowerFloat(), r_value);
			i2 = mpfr_cmp_q(o.internalUpperFloat(), r_value);
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) {
				i = mpfr_cmp(o.internalUpperFloat(), fl_value);
				i2 = mpfr_cmp(o.internalLowerFloat(), fu_value);
			} else {
				i = -mpfr_cmp_q(fl_value, o.internalRational());
				i2 = -mpfr_cmp_q(fu_value, o.internalRational());
			}
		} else {
			i = mpq_cmp(o.internalRational(), r_value);
			i2 = i;
		}
		if(i2 == 0 || i == 0) {
			if(i == 0) i = i2;
			if(i > 0) return COMPARISON_RESULT_EQUAL_OR_GREATER;
			else if(i < 0) return COMPARISON_RESULT_EQUAL_OR_LESS;
		} else if(i2 != i) {
			return COMPARISON_RESULT_UNKNOWN;
		}
		if(i == 0) return COMPARISON_RESULT_EQUAL;
		else if(i > 0) return COMPARISON_RESULT_GREATER;
		else return COMPARISON_RESULT_LESS;
	} else {
		return COMPARISON_RESULT_NOT_EQUAL;
	}
}
ComparisonResult Number::compareApproximately(const Number &o, int prec) const {
	if(n_type == NUMBER_TYPE_INFINITY || o.isInfinity()) return COMPARISON_RESULT_UNKNOWN;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) {
		if(o.isPlusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_LESS;
	}
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
		if(o.isMinusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_GREATER;
	}
	if(o.isPlusInfinity()) return COMPARISON_RESULT_GREATER;
	if(o.isMinusInfinity()) return COMPARISON_RESULT_LESS;
	int b = equalsApproximately(o, prec);
	if(b > 0) return COMPARISON_RESULT_EQUAL;
	else if(b < 0) return COMPARISON_RESULT_UNKNOWN;
	if(!isComplex() && !o.isComplex()) {
		int i = 0, i2 = 0;
		if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
			i = mpfr_cmp_q(o.internalLowerFloat(), r_value);
			i2 = mpfr_cmp_q(o.internalUpperFloat(), r_value);
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) {
				i = mpfr_cmp(o.internalUpperFloat(), fl_value);
				i2 = mpfr_cmp(o.internalLowerFloat(), fu_value);
			} else {
				i = -mpfr_cmp_q(fl_value, o.internalRational());
				i2 = -mpfr_cmp_q(fu_value, o.internalRational());
			}
		} else {
			i = mpq_cmp(o.internalRational(), r_value);
			i2 = i;
		}
		if(i2 == 0 || i == 0) {
			if(i == 0) i = i2;
			if(i > 0) return COMPARISON_RESULT_EQUAL_OR_GREATER;
			else if(i < 0) return COMPARISON_RESULT_EQUAL_OR_LESS;
		} else if(i2 != i) {
			return COMPARISON_RESULT_UNKNOWN;
		}
		if(i == 0) return COMPARISON_RESULT_EQUAL;
		else if(i > 0) return COMPARISON_RESULT_GREATER;
		else return COMPARISON_RESULT_LESS;
	} else {		
		return COMPARISON_RESULT_NOT_EQUAL;
	}
}
ComparisonResult Number::compareImaginaryParts(const Number &o) const {
	if(o.isComplex()) {
		if(!i_value) return COMPARISON_RESULT_NOT_EQUAL;
		return i_value->compareRealParts(*o.internalImaginary());
	} else if(isComplex()) {
		return COMPARISON_RESULT_NOT_EQUAL;
	}
	return COMPARISON_RESULT_EQUAL;
}
ComparisonResult Number::compareRealParts(const Number &o) const {
	if(n_type == NUMBER_TYPE_INFINITY || o.isInfinity()) return COMPARISON_RESULT_UNKNOWN;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) {
		if(o.isPlusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_LESS;
	}
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
		if(o.isMinusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_GREATER;
	}
	if(o.isPlusInfinity()) return COMPARISON_RESULT_GREATER;
	if(o.isMinusInfinity()) return COMPARISON_RESULT_LESS;
	int i = 0, i2 = 0;
	if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
		i = mpfr_cmp_q(o.internalLowerFloat(), r_value);
		i2 = mpfr_cmp_q(o.internalUpperFloat(), r_value);
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isFloatingPoint()) {
			i = mpfr_cmp(o.internalUpperFloat(), fl_value);
			i2 = mpfr_cmp(o.internalLowerFloat(), fu_value);
		} else {
			i = -mpfr_cmp_q(fl_value, o.internalRational());
			i2 = -mpfr_cmp_q(fu_value, o.internalRational());
		}
	} else {
		i = mpq_cmp(o.internalRational(), r_value);
		i2 = i;
	}
	if(i2 == 0 || i == 0) {
		if(i == 0) i = i2;
		if(i > 0) return COMPARISON_RESULT_EQUAL_OR_GREATER;
		else if(i < 0) return COMPARISON_RESULT_EQUAL_OR_LESS;
	} else if(i2 != i) {
		return COMPARISON_RESULT_UNKNOWN;
	}
	if(i == 0) return COMPARISON_RESULT_EQUAL;
	else if(i > 0) return COMPARISON_RESULT_GREATER;
	else return COMPARISON_RESULT_LESS;
}
bool Number::isGreaterThan(const Number &o) const {
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || n_type == NUMBER_TYPE_INFINITY || o.isInfinity() || o.isPlusInfinity()) return false;
	if(o.isMinusInfinity()) return true;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	if(isComplex() || o.isComplex()) return false;
	if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_q(o.internalUpperFloat(), r_value) < 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isFloatingPoint()) return mpfr_greater_p(fl_value, o.internalUpperFloat());
		else return mpfr_cmp_q(fl_value, o.internalRational()) > 0;
	}
	return mpq_cmp(r_value, o.internalRational()) > 0;
}
bool Number::isLessThan(const Number &o) const {
	if(o.isMinusInfinity() || o.isInfinity() || n_type == NUMBER_TYPE_INFINITY || n_type == NUMBER_TYPE_PLUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || o.isPlusInfinity()) return true;
	if(isComplex() || o.isComplex()) return false;
	if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_q(o.internalLowerFloat(), r_value) > 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isFloatingPoint()) return mpfr_less_p(fu_value, o.internalLowerFloat());
		else return mpfr_cmp_q(fu_value, o.internalRational()) < 0;
	}
	return mpq_cmp(r_value, o.internalRational()) < 0;
}
bool Number::isGreaterThanOrEqualTo(const Number &o) const {
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || n_type == NUMBER_TYPE_INFINITY || o.isInfinity() || o.isPlusInfinity()) return false;
	if(o.isMinusInfinity()) return true;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	if(!isComplex() && !o.isComplex()) {
		if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
			return mpfr_cmp_q(o.internalUpperFloat(), r_value) <= 0;
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) return mpfr_greaterequal_p(fl_value, o.internalUpperFloat());
			else return mpfr_cmp_q(fl_value, o.internalRational()) >= 0;
		}
		return mpq_cmp(r_value, o.internalRational()) >= 0;
	}
	return false;
}
bool Number::isLessThanOrEqualTo(const Number &o) const {
	if(o.isMinusInfinity() || o.isInfinity() || n_type == NUMBER_TYPE_INFINITY || n_type == NUMBER_TYPE_PLUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || o.isPlusInfinity()) return true;
	if(!isComplex() && !o.isComplex()) {
		if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
			return mpfr_cmp_q(o.internalLowerFloat(), r_value) >= 0;
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) return mpfr_lessequal_p(fu_value, o.internalLowerFloat());
			else return mpfr_cmp_q(fu_value, o.internalRational()) <= 0;
		}
		return mpq_cmp(r_value, o.internalRational()) <= 0;
	}
	return false;
}
bool Number::isGreaterThan(long int i) const {
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || n_type == NUMBER_TYPE_INFINITY) return false;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_si(fl_value, i) > 0;
	}
	return mpq_cmp_si(r_value, i, 1) > 0;
}
bool Number::isLessThan(long int i) const {
	if(n_type == NUMBER_TYPE_INFINITY || n_type == NUMBER_TYPE_PLUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_si(fu_value, i) < 0;
	}
	return mpq_cmp_si(r_value, i, 1) < 0;
}
bool Number::isGreaterThanOrEqualTo(long int i) const {
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || n_type == NUMBER_TYPE_INFINITY) return false;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_si(fl_value, i) >= 0;
	}
	return mpq_cmp_si(r_value, i, 1) >= 0;
}
bool Number::isLessThanOrEqualTo(long int i) const {
	if(n_type == NUMBER_TYPE_INFINITY || n_type == NUMBER_TYPE_PLUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_si(fu_value, i) <= 0;
	}
	return mpq_cmp_si(r_value, i, 1) <= 0;
}
bool Number::numeratorIsGreaterThan(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_numref(r_value), i) > 0;
}
bool Number::numeratorIsLessThan(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_numref(r_value), i) < 0;
}
bool Number::numeratorEquals(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_numref(r_value), i) == 0;
}
bool Number::denominatorIsGreaterThan(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_denref(r_value), i) > 0;
}
bool Number::denominatorIsLessThan(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_denref(r_value), i) < 0;
}
bool Number::denominatorEquals(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_denref(r_value), i) == 0;
}
bool Number::denominatorIsGreater(const Number &o) const {
	if(!isRational() || !o.isRational()) return false;
	return mpz_cmp(mpq_denref(r_value), mpq_denref(o.internalRational())) > 0;
}
bool Number::denominatorIsLess(const Number &o) const {
	if(!isRational() || !o.isRational()) return false;
	return mpz_cmp(mpq_denref(r_value), mpq_denref(o.internalRational())) < 0;
}
bool Number::denominatorIsEqual(const Number &o) const {
	if(!isRational() || !o.isRational()) return false;
	return mpz_cmp(mpq_denref(r_value), mpq_denref(o.internalRational())) == 0;
}
bool Number::isEven() const {
	return isInteger() && mpz_even_p(mpq_numref(r_value));
}
bool Number::denominatorIsEven() const {
	return !isInfinite() && !isComplex() && !isFloatingPoint() && mpz_even_p(mpq_denref(r_value));
}
bool Number::denominatorIsTwo() const {
	return !isInfinite() && !isComplex() && !isFloatingPoint() && mpz_cmp_si(mpq_denref(r_value), 2) == 0;
}
bool Number::numeratorIsEven() const {
	return !isInfinite() && !isComplex() && !isFloatingPoint() && mpz_even_p(mpq_numref(r_value));
}
bool Number::numeratorIsOne() const {
	return !isInfinite() && !isComplex() && !isFloatingPoint() && mpz_cmp_si(mpq_numref(r_value), 1) == 0;
}
bool Number::numeratorIsMinusOne() const {
	return !isInfinite() && !isComplex() && !isFloatingPoint() && mpz_cmp_si(mpq_numref(r_value), -1) == 0;
}
bool Number::isOdd() const {
	return isInteger() && mpz_odd_p(mpq_numref(r_value));
}

int Number::integerLength() const {
	if(isInteger()) return mpz_sizeinbase(mpq_numref(r_value), 2);
	return 0;
}


bool Number::add(const Number &o) {
	if(n_type == NUMBER_TYPE_INFINITY) return !o.isInfinite();
	if(o.isInfinity()) {
		if(b_imag) return false;
		if(isInfinite()) return false;
		setInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) return !o.isPlusInfinity();
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return !o.isMinusInfinity();
	if(o.isPlusInfinity()) {
		if(b_imag) return false;
		setPlusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusInfinity()) {
		if(b_imag) return false;
		setMinusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isComplex()) {
		if(!i_value) {i_value = new Number(*o.internalImaginary()); i_value->markAsImaginaryPart();}
		else if(!i_value->add(*o.internalImaginary())) return false;
		setPrecisionAndApproximateFrom(*i_value);
	}
	if(o.isFloatingPoint() || n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		mpfr_clear_flags();
		if(n_type != NUMBER_TYPE_FLOAT) {
			mpfr_init2(fu_value, BIT_PRECISION);
			mpfr_init2(fl_value, BIT_PRECISION);
			n_type = NUMBER_TYPE_FLOAT;
			if(!CALCULATOR->usesIntervalArithmetics() && !o.isInterval()) {
				mpfr_add_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				mpfr_add_q(fu_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
				mpfr_add_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDD);
			}
			mpq_set_ui(r_value, 0, 1);
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) {
				if(!CALCULATOR->usesIntervalArithmetics() && !isInterval() && !o.isInterval()) {
					mpfr_add(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_add(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
					mpfr_add(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDD);
				}
			} else {
				if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
					mpfr_add_q(fl_value, fl_value, o.internalRational(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_add_q(fu_value, fu_value, o.internalRational(), MPFR_RNDU);
					mpfr_add_q(fl_value, fl_value, o.internalRational(), MPFR_RNDD);
				}
			}
		}
		if(!testFloatResult(false)) {
			set(nr_bak);
			return false;
		}
	} else {
		mpq_add(r_value, r_value, o.internalRational());
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::add(long int i) {
	if(i == 0) return true;
	if(isInfinite()) return true;
	if(n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		mpfr_clear_flags();
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_add_si(fl_value, fl_value, i, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_add_si(fu_value, fu_value, i, MPFR_RNDU);
			mpfr_add_si(fl_value, fl_value, i, MPFR_RNDD);
		}
		if(!testFloatResult(false)) {
			set(nr_bak);
			return false;
		}
	} else {
		if(i < 0) mpz_submul_ui(mpq_numref(r_value), mpq_denref(r_value), (unsigned int) (-i));
		else mpz_addmul_ui(mpq_numref(r_value), mpq_denref(r_value), (unsigned int) i);
	}
	return true;
}

bool Number::subtract(const Number &o) {
	if(n_type == NUMBER_TYPE_INFINITY) {
		return !o.isInfinite();
	}
	if(o.isInfinity()) {
		if(b_imag) return false;
		if(isInfinite()) return false;
		setPrecisionAndApproximateFrom(o);
		setInfinity();
		return true;
	}
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) {
		return !o.isPlusInfinity();
	}
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
		return !o.isMinusInfinity();
	}
	if(o.isPlusInfinity()) {
		if(b_imag) return false;
		setMinusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusInfinity()) {
		if(b_imag) return false;
		setPlusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isComplex()) {
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		if(!i_value->subtract(*o.internalImaginary())) return false;
		setPrecisionAndApproximateFrom(*i_value);
	}
	if(o.isFloatingPoint() || n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		mpfr_clear_flags();
		if(n_type != NUMBER_TYPE_FLOAT) {
			mpfr_init2(fu_value, BIT_PRECISION);
			mpfr_init2(fl_value, BIT_PRECISION);
			n_type = NUMBER_TYPE_FLOAT;
			if(!CALCULATOR->usesIntervalArithmetics() && !o.isInterval()) {
				mpfr_sub_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDN);
				mpfr_neg(fl_value, fl_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				mpfr_sub_q(fu_value, o.internalLowerFloat(), r_value, MPFR_RNDD);
				mpfr_neg(fu_value, fu_value, MPFR_RNDU);
				mpfr_sub_q(fl_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
				mpfr_neg(fl_value, fl_value, MPFR_RNDD);
			}
			mpq_set_ui(r_value, 0, 1);
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) {
				if(!CALCULATOR->usesIntervalArithmetics() && !isInterval() && !o.isInterval()) {
					mpfr_sub(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_sub(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDU);
					mpfr_sub(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
				}
			} else {
				if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
					mpfr_sub_q(fl_value, fl_value, o.internalRational(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_sub_q(fu_value, fu_value, o.internalRational(), MPFR_RNDU);
					mpfr_sub_q(fl_value, fl_value, o.internalRational(), MPFR_RNDD);
				}
			}
		}
		if(!testFloatResult(false)) {
			set(nr_bak);
			return false;
		}
	} else {
		mpq_sub(r_value, r_value, o.internalRational());
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::subtract(long int i) {
	if(i == 0) return true;
	if(isInfinite()) return true;
	if(n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		mpfr_clear_flags();
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_sub_si(fl_value, fl_value, i, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_sub_si(fu_value, fu_value, i, MPFR_RNDU);
			mpfr_sub_si(fl_value, fl_value, i, MPFR_RNDD);
		}
		if(!testFloatResult(false)) {
			set(nr_bak);
			return false;
		}
	} else {
		if(i < 0) mpz_addmul_ui(mpq_numref(r_value), mpq_denref(r_value), (unsigned int) (-i));
		else mpz_submul_ui(mpq_numref(r_value), mpq_denref(r_value), (unsigned int) i);
	}
	return true;
}

bool Number::multiply(const Number &o) {
	if(o.isInfinite() && isZero()) return false;
	if(isInfinite() && o.isZero()) return false;
	if((isInfinite() && o.isComplex()) || (o.isInfinite() && isComplex())) {
		//setInfinity();
		//return true;
		return false;
	}
	if(isInfinity()) return true;
	if(o.isInfinity()) {
		setInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || n_type == NUMBER_TYPE_PLUS_INFINITY) {
		if(o.isNegative()) {
			if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
				n_type = NUMBER_TYPE_PLUS_INFINITY;
			} else {
				n_type = NUMBER_TYPE_MINUS_INFINITY;
			}
			setPrecisionAndApproximateFrom(o);
		}
		return true;
	}
	if(o.isPlusInfinity()) {
		if(b_imag) return false;
		if(isNegative()) setMinusInfinity();
		else setPlusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusInfinity()) {
		if(b_imag) return false;
		if(isNegative()) setPlusInfinity();
		else setMinusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(isZero()) return true;
	if(o.isZero()) {
		clear();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.hasImaginaryPart()) {
		if(o.hasRealPart()) {
			Number nr_bak(*this);
			Number nr_copy;
			if(hasImaginaryPart()) {
				nr_copy.set(*i_value);
				nr_copy.negate();
			}
			if(hasRealPart()) {
				nr_copy.setImaginaryPart(*this);
			}
			if(!nr_copy.multiply(o.imaginaryPart())) return false;
			if(!multiply(o.realPart())) return false;
			if(!add(nr_copy)) {
				set(nr_bak);
				return false;
			}
			return true;
		} else if(hasImaginaryPart()) {
			Number nr_copy(*i_value);
			nr_copy.negate();
			if(hasRealPart()) {
				nr_copy.setImaginaryPart(*this);
			}
			if(!nr_copy.multiply(o.imaginaryPart())) return false;
			set(nr_copy);
			return true;
		}
		if(!multiply(*o.internalImaginary())) return false;
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		i_value->set(*this, true, true);
		clearReal();
		return true;
	}
	if(o.isFloatingPoint() || n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		if(hasImaginaryPart()) {
			if(!i_value->multiply(o)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		mpfr_clear_flags();
		if(n_type != NUMBER_TYPE_FLOAT) {
			mpfr_init2(fu_value, BIT_PRECISION);
			mpfr_init2(fl_value, BIT_PRECISION);
			n_type = NUMBER_TYPE_FLOAT;
			if(!CALCULATOR->usesIntervalArithmetics() && !o.isInterval()) {
				mpfr_mul_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				if(mpq_sgn(r_value) < 0) {
					mpfr_mul_q(fu_value, o.internalLowerFloat(), r_value, MPFR_RNDU);
					mpfr_mul_q(fl_value, o.internalUpperFloat(), r_value, MPFR_RNDD);
				} else {
					mpfr_mul_q(fu_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
					mpfr_mul_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDD);
				}
			}
			mpq_set_ui(r_value, 0, 1);
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) {
				if(!CALCULATOR->usesIntervalArithmetics() && !isInterval() && !o.isInterval()) {
					mpfr_mul(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					int sgn_l = mpfr_sgn(fl_value), sgn_u = mpfr_sgn(fu_value), sgn_ol = mpfr_sgn(o.internalLowerFloat()), sgn_ou = mpfr_sgn(o.internalUpperFloat());
					if((sgn_l < 0) != (sgn_u < 0)) {
						if((sgn_ol < 0) != (sgn_ou < 0)) {
							mpfr_t fu_value2, fl_value2;
							mpfr_init2(fu_value, BIT_PRECISION);
							mpfr_init2(fl_value, BIT_PRECISION);
							mpfr_mul(fu_value2, fl_value, o.internalLowerFloat(), MPFR_RNDU);
							mpfr_mul(fl_value2, fu_value, o.internalLowerFloat(), MPFR_RNDD);
							mpfr_mul(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
							mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
							if(mpfr_cmp(fu_value, fu_value2) < 0) mpfr_set(fu_value, fu_value2, MPFR_RNDU);
							if(mpfr_cmp(fl_value, fl_value2) > 0) mpfr_set(fl_value, fl_value2, MPFR_RNDD);
							mpfr_clears(fu_value2, fl_value2, NULL);
						} else if(sgn_ol < 0) {
							mpfr_mul(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDU);
							mpfr_mul(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDD);
							mpfr_swap(fu_value, fl_value);
						} else {
							mpfr_mul(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
							mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
						}
					} else if((sgn_ol < 0) != (sgn_ou < 0)) {
						if(sgn_l < 0) {
							mpfr_mul(fu_value, fl_value, o.internalLowerFloat(), MPFR_RNDU);
							mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
						} else {
							mpfr_mul(fl_value, fu_value, o.internalLowerFloat(), MPFR_RNDD);
							mpfr_mul(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
						}
					} else if(sgn_l < 0) {
						if(sgn_ol < 0) {
							mpfr_mul(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDU);
							mpfr_mul(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDD);
							if(mpq_sgn(o.internalRational()) < 0) mpfr_swap(fu_value, fl_value);
						} else {
							mpfr_mul(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDU);
							mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
						}
					} else if(sgn_ol < 0) {
						mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDU);
						mpfr_mul(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDD);
						mpfr_swap(fu_value, fl_value);
					} else {
						mpfr_mul(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
						mpfr_mul(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDD);
					}
				}
			} else {
				if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
					mpfr_mul_q(fl_value, fl_value, o.internalRational(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_mul_q(fu_value, fu_value, o.internalRational(), MPFR_RNDU);
					mpfr_mul_q(fl_value, fl_value, o.internalRational(), MPFR_RNDD);
					if(mpq_sgn(o.internalRational()) < 0) mpfr_swap(fu_value, fl_value);
				}
			}
		}
		if(!testFloatResult(false)) {
			set(nr_bak);
			return false;
		}
	} else {
		if(hasImaginaryPart()) {
			if(!i_value->multiply(o)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		mpq_mul(r_value, r_value, o.internalRational());
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::multiply(long int i) {
	if(isInfinite() && i == 0) return false;
	if(isInfinity()) return true;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || n_type == NUMBER_TYPE_PLUS_INFINITY) {
		if(i < 0) {
			if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
				n_type = NUMBER_TYPE_PLUS_INFINITY;
			} else {
				n_type = NUMBER_TYPE_MINUS_INFINITY;
			}
		}
		return true;
	}
	if(isZero()) return true;
	if(i == 0) {
		clear();
		return true;
	}
	if(isInfinite()) return true;
	if(n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		if(hasImaginaryPart()) {
			if(!i_value->multiply(i)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		mpfr_clear_flags();

		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_mul_si(fl_value, fl_value, i, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_mul_si(fu_value, fu_value, i, MPFR_RNDU);
			mpfr_mul_si(fl_value, fl_value, i, MPFR_RNDD);
			if(i < 0) mpfr_swap(fu_value, fl_value);
		}
		
		if(!testFloatResult(false)) {
			set(nr_bak);
			return false;
		}
	} else {
		if(hasImaginaryPart()) {
			if(!i_value->multiply(i)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		mpq_t r_i;
		mpq_init(r_i);
		mpz_set_si(mpq_numref(r_i), i);
		mpq_mul(r_value, r_value, r_i);
		mpq_clear(r_i);
	}
	return true;
}

bool Number::divide(const Number &o) {
	if(isInfinite() && o.isInfinite()) return false;
	if(isInfinite() && o.isZero()) {
		//setInfinity();
		//return true;
		return false;
	}
	if(o.isInfinite()) {
		clear();
		return true;
	}
	if(isInfinite()) {
		if(o.isComplex()) {
			//setInfinity();
			return false;
		} else if(o.isNegative()) {
			if(n_type == NUMBER_TYPE_PLUS_INFINITY) {
				n_type = NUMBER_TYPE_MINUS_INFINITY;
			} else if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
				n_type = NUMBER_TYPE_PLUS_INFINITY;
			}
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isZero()) {
		if(isZero()) return false;
		//division by zero!!!
		//setInfinity();
		//return true;
		return false;
	}
	if(isZero()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isComplex() || o.isFloatingPoint() || n_type == NUMBER_TYPE_FLOAT) {
		Number oinv(o);
		if(!oinv.recip()) return false;
		return multiply(oinv);
	}
	if(isComplex()) {
		if(!i_value->divide(o)) return false;
		setPrecisionAndApproximateFrom(*i_value);
	}
	mpq_div(r_value, r_value, o.internalRational());
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::divide(long int i) {
	if(isInfinite() && i == 0) return false;
	if(isInfinite()) {
		if(i < 0) {
			if(n_type == NUMBER_TYPE_PLUS_INFINITY) {
				n_type = NUMBER_TYPE_MINUS_INFINITY;
			} else if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
				n_type = NUMBER_TYPE_PLUS_INFINITY;
			}
		}
		return true;
	}
	if(i == 0) return false;
	if(isZero()) return true;
	if(n_type == NUMBER_TYPE_FLOAT) {
		Number oinv(1, i, 0);
		return multiply(oinv);
	}
	if(isComplex()) {
		if(!i_value->divide(i)) return false;
		setPrecisionAndApproximateFrom(*i_value);
	}
	mpq_t r_i;
	mpq_init(r_i);
	mpz_set_si(mpq_numref(r_i), i);
	mpq_div(r_value, r_value, r_i);
	mpq_clear(r_i);
	return true;
}

bool Number::recip() {
	if(isZero()) {
		//division by zero!!!
		//setInfinity();
		//return true;
		return false;
	}
	if(isInfinite()) {
		clear();
		return true;
	}
	if(isComplex()) {
		if(hasRealPart()) {
			Number den1(*i_value);
			Number den2;
			den2.set(*this, false, true);
			Number num_r(den2), num_i(den1);
			if(!den1.square() || !num_i.negate() || !den2.square() || !den1.add(den2) || !num_r.divide(den1) || !num_i.divide(den1)) return false;
			set(num_r);
			setImaginaryPart(num_i);
			return true;
		} else {
			i_value->recip();
			i_value->negate();
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		}
	}
	if(n_type == NUMBER_TYPE_FLOAT) {
		if(mpfr_sgn(fu_value) != mpfr_sgn(fl_value)) return false;
		Number nr_bak(*this);
		
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_ui_div(fl_value, 1, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_ui_div(fu_value, 1, fu_value, MPFR_RNDD);
			mpfr_ui_div(fl_value, 1, fl_value, MPFR_RNDU);
			mpfr_swap(fu_value, fl_value);
		}
		
		if(!testFloatResult(false)) {
			set(nr_bak);
			return false;
		}
	} else {
		mpq_inv(r_value, r_value);
	}
	return true;
}
bool Number::raise(const Number &o, bool try_exact) {
	if(o.isInfinity()) return false;
	if(isInfinite()) {	
		if(o.isNegative()) {
			clear();
			return true;
		}
		if(o.isZero()) {
			return false;
		}
		if(isMinusInfinity()) {
			if(o.isEven()) {
				n_type = NUMBER_TYPE_PLUS_INFINITY;
			} else if(!o.isInteger()) {
				//setInfinity();
				return false;
			}
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusInfinity()) {
		if(isZero()) {
			//setInfinity();
			return false;
		} else if(isComplex()) {
			return false;
		} else {
			clear();
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isPlusInfinity()) {
		if(isZero()) {
		} else if(b_imag || isComplex() || isNegative()) {
			return false;
		} else {
			setPlusInfinity();
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(isZero() && o.isNegative()) {
		CALCULATOR->error(true, _("Division by zero."), NULL);
		return false;
	}
	if(isZero()) {
		if(o.isZero()) {
			//0^0
			CALCULATOR->error(false, _("0^0 might be considered undefined"), NULL);
			set(1, 1, 0, true);
			setPrecisionAndApproximateFrom(o);
			return true;
		} else if(o.isComplex()) {
			CALCULATOR->error(false, _("The result of 0^i is possibly undefined"), NULL);
		} else {
			return true;
		}
	}
	if(o.isOne()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusOne()) {
		if(!recip()) return false;
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isComplex()) {
		if(b_imag) return false;
		Number nr_a, nr_b, nr_c, nr_d;
		if(hasImaginaryPart()) {
			if(hasRealPart()) nr_a = realPart();
			nr_b = imaginaryPart();
		} else {
			nr_a.set(*this);
		}
		if(o.hasRealPart()) nr_c = o.realPart();
		nr_d = o.imaginaryPart();
		Number a2b2c2(1, 1);
		Number a2b2(nr_a);
		Number b2(nr_b);
		if(!a2b2.square() || !b2.square() || !a2b2.add(b2)) return false;
		if(!nr_c.isZero()) {
			Number chalf(nr_c);
			a2b2c2 = a2b2;
			if(!chalf.multiply(nr_half) || !a2b2c2.raise(chalf)) return false;
		}
		Number nr_arg(nr_b);
		if(!nr_arg.atan2(nr_a)) return false;
		Number eraised, nexp(nr_d);
		eraised.e();
		if(!nexp.negate() || !nexp.multiply(nr_arg) || !eraised.raise(nexp, false)) return false;
		
		if(!nr_arg.multiply(nr_c) || !nr_d.multiply(nr_half) || !a2b2.ln() || !nr_d.multiply(a2b2) || !nr_arg.add(nr_d)) return false;
		Number nr_cos(nr_arg);
		Number nr_sin(nr_arg);
		if(!nr_cos.cos() || !nr_sin.sin() || !nr_sin.multiply(nr_one_i) || !nr_cos.add(nr_sin)) return false;
		if(!eraised.multiply(a2b2c2) || !eraised.multiply(nr_cos)) return false;
		set(eraised);
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(isComplex()) {
		if(o.isNegative()) {
			if(o.isMinusOne()) return recip();
			Number ninv(*this), opos(o);
			if(!ninv.recip() ||!opos.negate() || !ninv.raise(opos, try_exact)) return false;
			set(ninv);
			return true;
		}
		if(hasRealPart() || !o.isInteger()) {
			if(try_exact && !isFloatingPoint() && !i_value->isFloatingPoint() && o.isInteger() && o.isLessThan(100)) {
				int i = o.intValue();
				Number nr_init(*this);
				while(i > 1) {
					if(CALCULATOR->aborted()) return false;
					if(!multiply(nr_init)) {
						set(nr_init);
						return false;
					}
					i--;
				}
				setPrecisionAndApproximateFrom(o);
				return true;
			}
			Number nbase, nexp(*this);
			nbase.e();
			if(!nexp.ln() || !nexp.multiply(o) || !nbase.raise(nexp, false)) return false;
			set(nbase);
			return true;
		}
		if(!i_value->raise(o, try_exact)) return false;
		Number ibneg(o);
		if(!ibneg.iquo(2)) return false;
		if(!ibneg.isEven() && !i_value->negate()) return false;
		if(o.isEven()) {
			set(*i_value, true, true);
			clearImaginary();
		} else {
			setPrecisionAndApproximateFrom(*i_value);
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}

	
	if(isMinusOne() && o.isRational()) {
		if(o.isInteger()) {
			if(o.isEven()) set(1, 1, 0, true);
			setPrecisionAndApproximateFrom(o);
			return true;
		} else if(o.denominatorIsTwo()) {
			if(b_imag) return false;
			clear(true);
			if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
			if(o.numeratorIsOne()) {
				i_value->set(1, 1, 0);
			} else {
				mpz_t zrem;
				mpz_init(zrem);
				mpz_tdiv_r_ui(zrem, mpq_numref(o.internalRational()), 4);
				if(mpz_cmp_ui(zrem, 1) == 0 || mpz_cmp_si(zrem, -3) == 0) {
					i_value->set(1, 1, 0);
				} else {
					i_value->set(-1, 1, 0);
				}
			}
			setPrecisionAndApproximateFrom(o);
			return true;
		}
	}

	if(n_type == NUMBER_TYPE_RATIONAL && !o.isFloatingPoint()) {
		bool success = false;
		if(mpz_fits_slong_p(mpq_numref(o.internalRational())) != 0 && mpz_fits_ulong_p(mpq_denref(o.internalRational())) != 0) {
			long int i_pow = mpz_get_si(mpq_numref(o.internalRational()));
			unsigned long int i_root = mpz_get_ui(mpq_denref(o.internalRational()));
			size_t length1 = mpz_sizeinbase(mpq_numref(r_value), 10);
			size_t length2 = mpz_sizeinbase(mpq_denref(r_value), 10);
			if(length2 > length1) length1 = length2;
			if((i_root <= 2  || mpq_sgn(r_value) > 0) && ((!try_exact && i_root <= 3 && (long long int) labs(i_pow) * length1 < 1000) || (try_exact && (long long int) labs(i_pow) * length1 < 1000000LL && i_root < 1000000L))) {
				bool complex_result = false;
				if(i_root != 1) {
					mpq_t r_test;
					mpq_init(r_test);
					if(i_pow < 0) {
						mpq_inv(r_test, r_value);
						i_pow = -i_pow;
					} else {
						mpq_set(r_test, r_value);
					}
					if(mpz_cmp_ui(mpq_denref(r_test), 1) == 0) {
						if(i_pow != 1) mpz_pow_ui(mpq_numref(r_test), mpq_numref(r_test), (unsigned long int) i_pow);
						if(i_root % 2 == 0 && mpq_sgn(r_test) < 0) {
							if(b_imag) {mpq_clear(r_test); return false;}
							if(i_root == 2) {
								mpq_neg(r_test, r_test);
								success = mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root);
								complex_result = true;
							}
						} else {
							success = mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root);
						}
					} else {
						if(i_pow != 1) {
							mpz_pow_ui(mpq_numref(r_test), mpq_numref(r_test), (unsigned long int) i_pow);
							mpz_pow_ui(mpq_denref(r_test), mpq_denref(r_test), (unsigned long int) i_pow);
						}
						if(i_root % 2 == 0 && mpq_sgn(r_test) < 0) {
							if(b_imag) {mpq_clear(r_test); return false;}
							if(i_root == 2) {
								mpq_neg(r_test, r_test);
								success = mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root) && mpz_root(mpq_denref(r_test), mpq_denref(r_test), i_root);
								complex_result = true;
							}
						} else {
							success = mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root) && mpz_root(mpq_denref(r_test), mpq_denref(r_test), i_root);
						}
					}
					if(success) {
						if(complex_result) {
							if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
							i_value->setInternal(r_test, false, true);
							if(i_pow % 4 == 3) i_value->negate();
							mpq_set_ui(r_value, 0, 1);
						} else {
							mpq_set(r_value, r_test);
						}
					}
					mpq_clear(r_test);
				} else if(i_pow != 1) {
					if(i_pow < 0) {
						mpq_inv(r_value, r_value);
						i_pow = -i_pow;
					}
					if(i_pow != 1) {
						mpz_pow_ui(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) i_pow);
						if(mpz_cmp_ui(mpq_denref(r_value), 1) != 0) mpz_pow_ui(mpq_denref(r_value), mpq_denref(r_value), (unsigned long int) i_pow);
					}
					success = true;
				} else {
					success = true;
				}
			}
		}
		if(success) {
			setPrecisionAndApproximateFrom(o);
			return true;
		}
	}
	Number nr_bak(*this);
	
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	
	int sgn_l = mpfr_sgn(fl_value), sgn_u = mpfr_sgn(fu_value);

	bool try_complex = false;
	if(o.isFloatingPoint()) {
		int sgn_ol = mpfr_sgn(o.internalLowerFloat()), sgn_ou = mpfr_sgn(o.internalUpperFloat());
		if(sgn_ol < 0 && (sgn_l == 0 || sgn_l != sgn_u)) {
			//CALCULATOR->error(true, _("Division by zero."), NULL);
			set(nr_bak);
			return false;
		}
		if(sgn_l < 0) {
			try_complex = true;
		} else {
			if(!CALCULATOR->usesIntervalArithmetics() && !isInterval() && !o.isInterval()) {
				mpfr_pow(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else if(sgn_ol < 0) {
				if(sgn_ou < 0) {
					mpfr_pow(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDD);
					mpfr_pow(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDU);
					mpfr_swap(fu_value, fl_value);
				} else {
					mpfr_t fu_value2, fl_value2;
					mpfr_init2(fu_value2, BIT_PRECISION);
					mpfr_init2(fl_value2, BIT_PRECISION);

					mpfr_pow(fu_value2, fu_value, o.internalUpperFloat(), MPFR_RNDU);
					mpfr_pow(fl_value2, fl_value, o.internalLowerFloat(), MPFR_RNDD);
					
					if(mpfr_cmp(fu_value2, fl_value2) < 0) {
						mpfr_pow(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDD);
						mpfr_pow(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDU);
						if(mpfr_cmp(fu_value, fl_value) < 0) mpfr_swap(fu_value, fl_value);
					} else {
						mpfr_set(fu_value, fu_value2, MPFR_RNDU);
						mpfr_set(fl_value, fl_value2, MPFR_RNDD);
					}
					
					mpfr_clears(fu_value2, fl_value2, NULL);
				}
			} else {
				mpfr_pow(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
				mpfr_pow(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDD);
			}
		}
	} else {
		if(o.isNegative() && (sgn_l == 0 || sgn_l != sgn_u)) {
			//CALCULATOR->error(true, _("Division by zero."), NULL);
			set(nr_bak);
			return false;
		}
		if(o.isInteger()) {
			if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
				mpfr_pow_z(fl_value, fl_value, mpq_numref(o.internalRational()), MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else if(o.isEven() && sgn_l < 0 && sgn_u >= 0) {
				if(mpfr_cmpabs(fu_value, fl_value) < 0) {
					mpfr_pow_z(fu_value, fl_value, mpq_numref(o.internalRational()), MPFR_RNDU);
				} else {
					mpfr_pow_z(fu_value, fu_value, mpq_numref(o.internalRational()), MPFR_RNDU);
				}
				mpfr_set_ui(fl_value, 0, MPFR_RNDD);
			} else {
				bool b_reverse = o.isEven() && sgn_l < 0;
				if(o.isNegative()) {
					if(b_reverse) {
						b_reverse = false;
					} else {
						b_reverse = true;
					}
				}
				if(b_reverse) {
					mpfr_pow_z(fu_value, fu_value, mpq_numref(o.internalRational()), MPFR_RNDD);
					mpfr_pow_z(fl_value, fl_value, mpq_numref(o.internalRational()), MPFR_RNDU);
					mpfr_swap(fu_value, fl_value);
				} else {
					mpfr_pow_z(fu_value, fu_value, mpq_numref(o.internalRational()), MPFR_RNDU);
					mpfr_pow_z(fl_value, fl_value, mpq_numref(o.internalRational()), MPFR_RNDD);
				}
			}
		} else {
			if(sgn_l < 0) {
				if(sgn_u < 0 && mpz_cmp_ui(mpq_denref(o.internalRational()), 2) == 0) {
					if(b_imag) {set(nr_bak); return false;}
					if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
					i_value->set(*this, false, true);
					if(!i_value->negate() || !i_value->raise(o)) {
						set(nr_bak);
						return false;
					}
					if(!o.numeratorIsOne()) {
						mpz_t zrem;
						mpz_init(zrem);
						mpz_tdiv_r_ui(zrem, mpq_numref(o.internalRational()), 4);
						if(mpz_cmp_ui(zrem, 1) != 0 && mpz_cmp_si(zrem, -3) != 0) i_value->negate();
					}
					clearReal();
					setPrecisionAndApproximateFrom(*i_value);
					return true;
				} else {
					try_complex = true;
				}
			} else {
				if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
					mpfr_t f_pow;
					mpfr_init2(f_pow, BIT_PRECISION);
					mpfr_set_q(f_pow, o.internalRational(), MPFR_RNDN);
					mpfr_pow(fl_value, fl_value, f_pow, MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
					mpfr_clears(f_pow, NULL);
				} else {
					mpfr_t f_pow_u, f_pow_l;
					mpfr_init2(f_pow_u, BIT_PRECISION);
					mpfr_init2(f_pow_l, BIT_PRECISION);
					mpfr_set_q(f_pow_u, o.internalRational(), MPFR_RNDU);
					mpfr_set_q(f_pow_l, o.internalRational(), MPFR_RNDD);
					if(o.isNegative()) {
						mpfr_pow(fu_value, fu_value, f_pow_u, MPFR_RNDD);
						mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDU);
						mpfr_swap(fu_value, fl_value);
					} else {
						mpfr_pow(fu_value, fu_value, f_pow_u, MPFR_RNDU);
						mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDD);
					}
					mpfr_clears(f_pow_u, f_pow_l, NULL);
				}
			}
		}
	}
	if(try_complex) {
		set(nr_bak);
		Number nbase, nexp(*this);
		nbase.e();
		if(!nexp.ln()) return false;
		if(!nexp.multiply(o)) return false;
		if(!nbase.raise(nexp, false)) return false;
		set(nbase);
		if(isComplex() && b_imag) {
			set(nr_bak);
			return false;
		}
		return true;
	}
	
	if(!testFloatResult(false)) {
		set(nr_bak);
		return false;
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::sqrt() {
	if(!isReal()) return raise(Number(1, 2, 0), true);
	if(isNegative()) {
		if(b_imag) return false;
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		i_value->set(*this, false, true);
		if(!i_value->negate() || !i_value->sqrt()) {
			i_value->clear();
			return false;
		}
		clearReal();
		setPrecisionAndApproximateFrom(*i_value);
		return true;
	}
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_perfect_square_p(mpq_numref(r_value)) && mpz_perfect_square_p(mpq_denref(r_value))) {
			mpz_sqrt(mpq_numref(r_value), mpq_numref(r_value));
			mpz_sqrt(mpq_denref(r_value), mpq_denref(r_value));
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();

	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_sqrt(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_sqrt(fu_value, fu_value, MPFR_RNDU);
		mpfr_sqrt(fl_value, fl_value, MPFR_RNDD);
	}
	
	if(!testFloatResult(false)) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::cbrt() {
	if(!isReal()) return raise(Number(1, 3, 0), true);
	if(isOne() || isMinusOne() || isZero()) return true;
	Number nr_bak(*this);
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_root(mpq_numref(r_value), mpq_numref(r_value), 3) && mpz_root(mpq_denref(r_value), mpq_denref(r_value), 3)) {
			return true;
		}
		set(nr_bak);
		if(!setToFloatingPoint()) return false;
	}
	mpfr_clear_flags();
	
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_cbrt(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_cbrt(fu_value, fu_value, MPFR_RNDU);
		mpfr_cbrt(fl_value, fl_value, MPFR_RNDD);
	}
	
	if(!testFloatResult(false)) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::root(const Number &o) {
	if(!o.isInteger() || !o.isPositive() || isComplex() || (o.isEven() && isNegative())) return false;
	if(isOne() || o.isOne() || isZero()) return true;
	if(o.isTwo()) return sqrt();
	if(isInfinite()) {
		Number o_inv(o);
		if(!o_inv.recip()) return false;
		return raise(o_inv, true);
	}
	/*if(o.isEven() && (!isReal() || isNegative())) {
		Number o_odd_factor(o);
		Number o_even_factor(1, 1, 0);
		while(o_odd_factor.isEven() && !o_odd_factor.isTwo()) {
			if(!o_odd_factor.multiply(nr_half) || !o_even_factor.multiply(2)) return false;
			if(CALCULATOR->aborted()) return false;
		}
		if(!o_even_factor.recip()) return false;
		Number nr_bak(*this);
		if(!root(o_odd_factor)) return false;
		if(!raise(o_even_factor)) {set(nr_bak); return false;}
		return true;
	}
	if(!isReal()) {
		if(!hasRealPart()) {
			Number nr_o(o);
			if(!nr_o.irem(4) || !i_value->root(o)) return false;
			if(!nr_o.isOne()) i_value->negate();
			return true;
		}
		return false;
	}*/
	if(isMinusOne()) return true;
	Number nr_bak(*this);
	if(!mpz_fits_ulong_p(mpq_numref(o.internalRational()))) {
		
		if(!setToFloatingPoint()) return false;
		
		Number o_inv(o);
		o_inv.recip();
		
		mpfr_t f_pow_u, f_pow_l;
		
		mpfr_init2(f_pow_u, BIT_PRECISION);
		mpfr_init2(f_pow_l, BIT_PRECISION);
		
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_set_q(f_pow_l, o.internalRational(), MPFR_RNDN);
			int sgn_l = mpfr_sgn(fl_value);
			if(sgn_l < 0) mpfr_neg(fl_value, fl_value, MPFR_RNDN);
			mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDN);
			if(sgn_l < 0) mpfr_neg(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_set_q(f_pow_u, o.internalRational(), MPFR_RNDU);
			mpfr_set_q(f_pow_l, o.internalRational(), MPFR_RNDD);
			
			int sgn_l = mpfr_sgn(fl_value), sgn_u = mpfr_sgn(fu_value);
			
			if(sgn_u < 0) mpfr_neg(fu_value, fu_value, MPFR_RNDD);
			if(sgn_l < 0) mpfr_neg(fl_value, fl_value, MPFR_RNDU);
			
			mpfr_pow(fu_value, fu_value, f_pow_u, MPFR_RNDU);
			mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDD);
			
			if(sgn_u < 0) mpfr_neg(fu_value, fu_value, MPFR_RNDU);
			if(sgn_l < 0) mpfr_neg(fl_value, fl_value, MPFR_RNDD);
		}
	
		mpfr_clears(f_pow_u, f_pow_l, NULL);
	} else {
		unsigned long int i_root = mpz_get_ui(mpq_numref(o.internalRational()));
		if(n_type == NUMBER_TYPE_RATIONAL) {
			if(mpz_root(mpq_numref(r_value), mpq_numref(r_value), i_root) && mpz_root(mpq_denref(r_value), mpq_denref(r_value), i_root)) {
				return true;
			}
			set(nr_bak);
			if(!setToFloatingPoint()) return false;
		}
		mpfr_clear_flags();
	
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_root(fl_value, fl_value, i_root, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_root(fu_value, fu_value, i_root, MPFR_RNDU);
			mpfr_root(fl_value, fl_value, i_root, MPFR_RNDD);
		}
	}
	
	if(!testFloatResult(false)) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::allroots(const Number &o, vector<Number> &roots) {
	if(!o.isInteger() || !o.isPositive()) return false;
	if(isOne() || o.isOne() || isZero()) {
		roots.push_back(*this);
		return true;
	}
	if(o.isTwo()) {
		Number nr(*this);
		if(!nr.sqrt()) return false;
		roots.push_back(nr);
		if(!nr.negate()) return false;
		roots.push_back(nr);
		return true;
	}
	if(isInfinite()) return false;
	Number o_inv(o);
	if(!o_inv.recip()) return false;
	Number nr_arg;
	nr_arg.set(*this, false, true);
	if(!nr_arg.atan2(*i_value)) return false;
	Number nr_pi2;
	nr_pi2.pi();
	nr_pi2 *= 2;
	Number nr_i;
	Number nr_re;
	nr_re.set(*this, false, true);
	Number nr_im(*i_value);
	if(!nr_re.square() || !nr_im.square() || !nr_re.add(nr_im) || !nr_re.sqrt() || !nr_re.raise(o_inv)) return false;
	while(nr_i.isLessThan(o)) {
		if(CALCULATOR->aborted()) return false;
		Number nr(nr_pi2);
		if(!nr.multiply(nr_i) || !nr.add(nr_arg) || !nr.multiply(nr_one_i) || !nr.multiply(o_inv) || !nr.exp() || !nr.multiply(nr_re)) return false;
		roots.push_back(nr);
		nr_i++;
	}
	return true;
}
bool Number::exp10(const Number &o) {
	if(isZero()) return true;
	if(o.isZero()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	Number ten(10, 1);
	if(!ten.raise(o)) {
		return false;
	}
	multiply(ten);
	return true;
}
bool Number::exp10() {
	if(isZero()) {
		set(1, 1);
		return true;
	}
	Number ten(10, 1);
	if(!ten.raise(*this)) {
		return false;
	}
	set(ten);
	return true;
}
bool Number::exp2(const Number &o) {
	if(isZero()) return true;
	if(o.isZero()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	Number two(2, 1);
	if(!two.raise(o)) {
		return false;
	}
	multiply(two);
	return true;
}
bool Number::exp2() {
	if(isZero()) {
		set(1, 1);
		return true;
	}
	Number two(2, 1);
	if(!two.raise(*this)) {
		return false;
	}
	set(two);
	return true;
}
bool Number::square() {
	if(isInfinite()) {
		n_type = NUMBER_TYPE_PLUS_INFINITY;
		return true;
	}
	if(isComplex()) {
		Number nr(*this);
		return multiply(nr);
	}
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpq_mul(r_value, r_value, r_value);
	} else {
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_sqr(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else if(mpfr_sgn(fl_value) < 0 && (mpfr_sgn(fu_value) <= 0 || mpfr_cmpabs(fu_value, fl_value) < 0)) {
			mpfr_sqr(fu_value, fu_value, MPFR_RNDD);
			mpfr_sqr(fl_value, fl_value, MPFR_RNDU);
			mpfr_swap(fu_value, fl_value);
		} else {
			mpfr_sqr(fu_value, fu_value, MPFR_RNDU);
			mpfr_sqr(fl_value, fl_value, MPFR_RNDD);
		}
	}
	return true;
}

bool Number::negate() {
	if(i_value) i_value->negate();
	switch(n_type) {
		case NUMBER_TYPE_PLUS_INFINITY: {
			n_type = NUMBER_TYPE_MINUS_INFINITY;
			break;
		}
		case NUMBER_TYPE_MINUS_INFINITY: {
			n_type = NUMBER_TYPE_PLUS_INFINITY;
			break;
		}
		case NUMBER_TYPE_RATIONAL: {
			mpq_neg(r_value, r_value);
			break;
		}
		case NUMBER_TYPE_FLOAT: {
			mpfr_clear_flags();
			if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
				mpfr_neg(fl_value, fl_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				mpfr_neg(fu_value, fu_value, MPFR_RNDD);
				mpfr_neg(fl_value, fl_value, MPFR_RNDU);
				mpfr_swap(fu_value, fl_value);
			}
			testFloatResult(false, 2);
			break;
		}
		default: {break;}
	}
	return true;
}
void Number::setNegative(bool is_negative) {
	switch(n_type) {
		case NUMBER_TYPE_PLUS_INFINITY: {
			if(is_negative) n_type = NUMBER_TYPE_MINUS_INFINITY;
			break;
		}
		case NUMBER_TYPE_MINUS_INFINITY: {
			if(!is_negative) n_type = NUMBER_TYPE_PLUS_INFINITY;
			break;
		}
		case NUMBER_TYPE_RATIONAL: {
			if(is_negative != (mpq_sgn(r_value) < 0)) mpq_neg(r_value, r_value);
			break;
		}
		case NUMBER_TYPE_FLOAT: {
			mpfr_clear_flags();
			if(mpfr_sgn(fl_value) != mpfr_sgn(fu_value)) {
				if(is_negative) {
					mpfr_neg(fu_value, fu_value, MPFR_RNDU);
					if(mpfr_cmp(fl_value, fu_value) < 0) mpfr_swap(fu_value, fl_value);
					mpfr_set_zero(fu_value, 0);
				} else {
					mpfr_abs(fl_value, fl_value, MPFR_RNDU);
					if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fu_value, fl_value);
					mpfr_set_zero(fl_value, 0);
				}
			} else if(is_negative != (mpfr_sgn(fl_value) < 0)) {
				if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
					mpfr_neg(fl_value, fl_value, MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_neg(fu_value, fu_value, MPFR_RNDD);
					mpfr_neg(fl_value, fl_value, MPFR_RNDU);
					mpfr_swap(fu_value, fl_value);
				}
				testFloatResult(false, 2);
			}
			break;
		}
		default: {break;}
	}
}
bool Number::abs() {
	if(isInfinite()) {
		n_type = NUMBER_TYPE_PLUS_INFINITY;
		return true;
	}
	if(isComplex()) {
		if(hasRealPart()) {
			Number nr_bak(*this);
			if(!i_value->square()) return false;
			Number *i_v = i_value;
			i_value = NULL;
			if(!square() || !add(*i_v)) {
				set(nr_bak);
				return false;
			}
			i_v->clear();
			i_value = i_v;
			if(!raise(nr_half)) {
				set(nr_bak);
				return false;
			}
			return true;
		}
		set(*i_value, true, true);
		clearImaginary();
	}
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpq_abs(r_value, r_value);
	} else {
		if(mpfr_sgn(fl_value) != mpfr_sgn(fu_value)) {
			mpfr_abs(fl_value, fl_value, MPFR_RNDU);
			if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fu_value, fl_value);
			mpfr_set_zero(fl_value, 0);
		} else if(mpfr_sgn(fl_value) < 0) {
			if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
				mpfr_neg(fl_value, fl_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				mpfr_neg(fu_value, fu_value, MPFR_RNDD);
				mpfr_neg(fl_value, fl_value, MPFR_RNDU);
				mpfr_swap(fu_value, fl_value);
			}
			testFloatResult(false, 2);
		}
	}
	return true;
}
bool Number::signum() {
	if(isZero()) return true;
	if(isComplex()) {
		if(hasRealPart()) {
			Number nabs(*this);
			if(!nabs.abs() || !nabs.recip()) return false;
			return multiply(nabs);
		}
		return i_value->signum();
	}
	if(isPositive()) {set(1, 1); return true;}
	if(isNegative()) {set(-1, 1); return true;}
	return false;
}
bool Number::round() {
	if(isInfinite() || isComplex()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_t i_rem;
			mpz_init(i_rem);
			mpz_mul_ui(mpq_numref(r_value), mpq_numref(r_value), 2);
			mpz_add(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_mul_ui(mpq_denref(r_value), mpq_denref(r_value), 2);
			mpz_fdiv_qr(mpq_numref(r_value), i_rem, mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
			if(mpz_sgn(i_rem) == 0 && mpz_odd_p(mpq_numref(r_value))) {
				if(mpz_sgn(mpq_numref(r_value)) < 0) mpz_add(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
				else mpz_sub(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			}
			mpz_clear(i_rem);
		}
	} else {
		mpz_set_ui(mpq_denref(r_value), 1);

		mpfr_t f_mid;
		mpfr_init2(f_mid, BIT_PRECISION);
		mpfr_sub(f_mid, fu_value, fl_value, MPFR_RNDN);
		mpfr_div_ui(f_mid, f_mid, 2, MPFR_RNDN);
		mpfr_add(f_mid, fl_value, f_mid, MPFR_RNDN);
		mpfr_get_z(mpq_numref(r_value), f_mid, MPFR_RNDN);
		n_type = NUMBER_TYPE_RATIONAL;
		mpfr_clears(f_mid, fl_value, fu_value, NULL);
	}
	return true;
}
bool Number::floor() {
	if(isInfinite() || isComplex()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_fdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
		}
	} else {
		mpz_set_ui(mpq_denref(r_value), 1);
		mpfr_get_z(mpq_numref(r_value), fl_value, MPFR_RNDD);
		n_type = NUMBER_TYPE_RATIONAL;
		mpfr_clears(fl_value, fu_value, NULL);
	}
	return true;
}
bool Number::ceil() {
	if(isInfinite() || isComplex()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_cdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
		}
	} else {
		mpz_set_ui(mpq_denref(r_value), 1);
		mpfr_get_z(mpq_numref(r_value), fu_value, MPFR_RNDU);
		n_type = NUMBER_TYPE_RATIONAL;
		mpfr_clears(fl_value, fu_value, NULL);
	}
	return true;
}
bool Number::trunc() {
	if(isInfinite() || isComplex()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_tdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
		}
	} else {
		mpz_set_ui(mpq_denref(r_value), 1);
		if(mpfr_sgn(fu_value) <= 0) mpfr_get_z(mpq_numref(r_value), fu_value, MPFR_RNDU);
		else if(mpfr_sgn(fl_value) >= 0) mpfr_get_z(mpq_numref(r_value), fl_value, MPFR_RNDD);
		else mpz_set_ui(mpq_numref(r_value), 0);
		n_type = NUMBER_TYPE_RATIONAL;
		mpfr_clears(fl_value, fu_value, NULL);
	}
	return true;
}
bool Number::round(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && round();
	}
	if(isComplex()) return false;
	if(o.isComplex()) return false;
	return divide(o) && round();
}
bool Number::floor(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && floor();
	}
	if(isComplex()) return false;
	if(o.isComplex()) return false;
	return divide(o) && floor();
}
bool Number::ceil(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && ceil();
	}
	if(isComplex()) return false;
	if(o.isComplex()) return false;
	return divide(o) && ceil();
}
bool Number::trunc(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && trunc();
	}
	if(isComplex()) return false;
	if(o.isComplex()) return false;
	return divide(o) && trunc();
}
bool Number::mod(const Number &o) {
	if(isInfinite() || o.isInfinite()) return false;
	if(isComplex() || o.isComplex()) return false;
	if(o.isZero()) return false;
	if(isRational() && o.isRational()) {
		if(isInteger() && o.isInteger()) {
			mpz_fdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
		} else {
			mpq_div(r_value, r_value, o.internalRational());
			mpz_fdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpq_mul(r_value, r_value, o.internalRational());
		}
	} else {
		// TODO: Interval too wide when o is interval
		if(!divide(o) || !frac()) return false;
		if(isNegative()) {
			(*this)++;
			testFloatResult(false, 2);
		}
		return multiply(o);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::frac() {
	if(isInfinite() || isComplex()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(isInteger()) {
			clear();
		} else {
			mpz_tdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
		}
	} else {
		mpfr_clear_flags();
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_frac(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else if(!isInterval()) {
			mpfr_frac(fl_value, fl_value, MPFR_RNDD);
			mpfr_frac(fu_value, fu_value, MPFR_RNDU);
		} else {
			mpfr_t testf, testu;
			mpfr_inits2(mpfr_get_prec(fl_value), testf, testu, NULL);
			mpfr_trunc(testf, fl_value);
			mpfr_trunc(testu, fu_value);
			if(!mpfr_equal_p(testf, testu)) {
				mpfr_set_zero(fl_value, 0);
				mpfr_set_ui(fu_value, 1, MPFR_RNDU);
			} else {
				mpfr_frac(testf, fl_value, MPFR_RNDU);
				mpfr_frac(testu, fu_value, MPFR_RNDU);
				if(mpfr_cmp(testf, testu) > 0) {
					mpfr_frac(fu_value, fl_value, MPFR_RNDU);
					mpfr_frac(fl_value, fu_value, MPFR_RNDD);
				} else {
					mpfr_frac(fl_value, fl_value, MPFR_RNDD);
					mpfr_frac(fu_value, fu_value, MPFR_RNDU);
				}
			}
			mpfr_clears(testf, testu, NULL);
		}
		testFloatResult(false, 2);
	}
	return true;
}
bool Number::rem(const Number &o) {
	if(isInfinite() || o.isInfinite()) return false;
	if(isComplex() || o.isComplex()) return false;
	if(o.isZero()) return false;
	if(isRational() && o.isRational()) {
		if(isInteger() && o.isInteger()) {
			mpz_tdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
		} else {
			mpq_div(r_value, r_value, o.internalRational());
			mpz_tdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpq_mul(r_value, r_value, o.internalRational());
		}
	} else {
		// TODO: Interval too wide when o is interval
		return divide(o) && frac() && multiply(o);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}

bool Number::smod(const Number &o) {
	if(!isInteger() || !o.isInteger()) return false;
	mpz_t b2;
	mpz_init(b2);
	mpz_div_ui(b2, mpq_numref(o.internalRational()), 2);
	mpz_sub_ui(b2, b2, 1);
	mpz_add(mpq_numref(r_value), mpq_numref(r_value), b2);
	mpz_fdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	mpz_sub(mpq_numref(r_value), mpq_numref(r_value), b2);
	mpz_clear(b2);
	setPrecisionAndApproximateFrom(o);
	return true;
}	
bool Number::irem(const Number &o) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	mpz_tdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	return true;
}
bool Number::irem(const Number &o, Number &q) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	q.set(1, 0);
	mpz_tdiv_qr(mpq_numref(q.internalRational()), mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	return true;
}
bool Number::iquo(const Number &o) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	mpz_tdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	return true;
}
bool Number::iquo(unsigned long int i) {
	if(i == 0) return false;
	if(!isInteger()) return false;
	mpz_tdiv_q_ui(mpq_numref(r_value), mpq_numref(r_value), i);
	return true;
}
bool Number::iquo(const Number &o, Number &r) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	r.set(1, 0);
	mpz_tdiv_qr(mpq_numref(r_value), mpq_numref(r.internalRational()), mpq_numref(r_value), mpq_numref(o.internalRational()));
	return true;
}
bool Number::isIntegerDivisible(const Number &o) const {
	if(!isInteger() || !o.isInteger()) return false;
	return mpz_divisible_p(mpq_numref(r_value), mpq_numref(o.internalRational()));
}
bool Number::isqrt() {
	if(isInteger()) {
		if(mpz_sgn(mpq_numref(r_value)) < 0) return false;
		mpz_sqrt(mpq_numref(r_value), mpq_numref(r_value));
		return true;
	}
	return false;
}
bool Number::isPerfectSquare() const {
	if(isInteger()) {
		if(mpz_sgn(mpq_numref(r_value)) < 0) return false;
		return mpz_perfect_square_p(mpq_numref(r_value)) != 0;
	}
	return false;
}

int Number::getBoolean() const {
	if(isPositive()) {
		return 1;
	} else if(isNonPositive()) {
		return 0;
	}
	return -1;
}
void Number::toBoolean() {
	setTrue(isPositive());
}
void Number::setTrue(bool is_true) {
	if(is_true) {
		set(1, 0);
	} else {
		clear();
	}
}
void Number::setFalse() {
	setTrue(false);
}
void Number::setLogicalNot() {
	setTrue(!isPositive());
}

void Number::e(bool use_cached_number) {
	if(use_cached_number) {
		if(nr_e.isZero() || CALCULATOR->usesIntervalArithmetics() != isInterval() || mpfr_get_prec(nr_e.internalLowerFloat()) != BIT_PRECISION) {
			nr_e.e(false);
		}
		set(nr_e);
	} else {
		if(n_type != NUMBER_TYPE_FLOAT) {
			mpfr_init2(fu_value, BIT_PRECISION); 
			mpfr_init2(fl_value, BIT_PRECISION); 
			mpq_set_ui(r_value, 0, 1);
		} else {
			if(mpfr_get_prec(fu_value) < BIT_PRECISION) mpfr_set_prec(fu_value, BIT_PRECISION);
			if(mpfr_get_prec(fl_value) < BIT_PRECISION) mpfr_set_prec(fl_value, BIT_PRECISION);
		}
		n_type = NUMBER_TYPE_FLOAT;
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_set_ui(fl_value, 1, MPFR_RNDN);
			mpfr_exp(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
			i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
		} else {
			mpfr_set_ui(fl_value, 1, MPFR_RNDD);
			mpfr_set_ui(fu_value, 1, MPFR_RNDU);
			mpfr_exp(fu_value, fu_value, MPFR_RNDU);
			mpfr_exp(fl_value, fl_value, MPFR_RNDD);
		}
	}
	b_approx = true;
}
void Number::pi() {
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_init2(fu_value, BIT_PRECISION); 
		mpfr_init2(fl_value, BIT_PRECISION); 
		mpq_set_ui(r_value, 0, 1);
	} else {
		if(mpfr_get_prec(fu_value) < BIT_PRECISION) mpfr_set_prec(fu_value, BIT_PRECISION);
		if(mpfr_get_prec(fl_value) < BIT_PRECISION) mpfr_set_prec(fl_value, BIT_PRECISION);
	}
	n_type = NUMBER_TYPE_FLOAT;
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_const_pi(fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
		i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
	} else {
		mpfr_const_pi(fu_value, MPFR_RNDU);
		mpfr_const_pi(fl_value, MPFR_RNDD);
	}
	b_approx = true;
}
void Number::catalan() {
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_init2(fu_value, BIT_PRECISION); 
		mpfr_init2(fl_value, BIT_PRECISION); 
		mpq_set_ui(r_value, 0, 1);
	} else {
		if(mpfr_get_prec(fu_value) < BIT_PRECISION) mpfr_set_prec(fu_value, BIT_PRECISION);
		if(mpfr_get_prec(fl_value) < BIT_PRECISION) mpfr_set_prec(fl_value, BIT_PRECISION);
	}
	n_type = NUMBER_TYPE_FLOAT;
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_const_catalan(fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
		i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
	} else {
		mpfr_const_catalan(fu_value, MPFR_RNDU);
		mpfr_const_catalan(fl_value, MPFR_RNDD);
	}
	b_approx = true;
}
void Number::euler() {
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_init2(fu_value, BIT_PRECISION); 
		mpfr_init2(fl_value, BIT_PRECISION); 
		mpq_set_ui(r_value, 0, 1);
	} else {
		if(mpfr_get_prec(fu_value) < BIT_PRECISION) mpfr_set_prec(fu_value, BIT_PRECISION);
		if(mpfr_get_prec(fl_value) < BIT_PRECISION) mpfr_set_prec(fl_value, BIT_PRECISION);
	}
	n_type = NUMBER_TYPE_FLOAT;
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_const_euler(fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
		i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
	} else {
		mpfr_const_euler(fu_value, MPFR_RNDU);
		mpfr_const_euler(fl_value, MPFR_RNDD);
	}
	b_approx = true;
}
bool Number::zeta() {
	if(isOne()) {
		setInfinity(true);
		return true;
	}
	if(isNegative() || !isInteger() || isZero()) {
		CALCULATOR->error(true, _("Can only handle Riemann Zeta with an integer argument (s) >= 1"), NULL);
		return false;
	}
	bool overflow = false;
	long int i = lintValue(&overflow);
	if(overflow) {
		CALCULATOR->error(true, _("Cannot handle an argument (s) that large for Riemann Zeta."), NULL);
		return false;
	}
	
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_zeta_ui(fl_value, (unsigned long int) i, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_zeta_ui(fu_value, (unsigned long int) i, MPFR_RNDU);
		mpfr_zeta_ui(fl_value, (unsigned long int) i, MPFR_RNDD);
	}
	
	mpq_set_ui(r_value, 0, 1);
	n_type = NUMBER_TYPE_FLOAT;
	
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	
	return true;
}
bool Number::gamma() {
	if(!isReal()) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_gamma(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_gamma(fu_value, fu_value, MPFR_RNDU);
		mpfr_gamma(fl_value, fl_value, MPFR_RNDD);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::digamma() {
	if(!isReal()) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_digamma(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_digamma(fu_value, fu_value, MPFR_RNDU);
		mpfr_digamma(fl_value, fl_value, MPFR_RNDD);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::erf() {
	if(!isReal()) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_erf(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_erf(fu_value, fu_value, MPFR_RNDU);
		mpfr_erf(fl_value, fl_value, MPFR_RNDD);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::erfc() {
	if(!isReal()) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_erfc(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_erfc(fu_value, fu_value, MPFR_RNDD);
		mpfr_erfc(fl_value, fl_value, MPFR_RNDU);
		mpfr_swap(fu_value, fl_value);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::airy() {
	if(!isReal()) return false;
	if(isGreaterThan(500) || isLessThan(-500)) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_ai(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_ai(fl_value, fl_value, MPFR_RNDN);
		mpfr_ai(fu_value, fu_value, MPFR_RNDN);
		if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
		CALCULATOR->error(false, _("%s() lacks proper support interval arithmetic."), CALCULATOR->f_airy->name());
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::besselj(const Number &o) {
	if(isComplex() || !o.isInteger()) return false;
	if(isZero()) return true;
	if(isInfinite()) {
		clear(true);
		return true;
	}
	if(!mpz_fits_slong_p(mpq_numref(o.internalRational()))) return false;
	long int n = mpz_get_si(mpq_numref(o.internalRational()));
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	if(mpfr_get_exp(fl_value) > 2000000L) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_jn(fl_value, n, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_jn(fl_value, n, fl_value, MPFR_RNDN);
		mpfr_jn(fu_value, n, fu_value, MPFR_RNDN);
		if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
		CALCULATOR->error(false, _("%s() lacks proper support interval arithmetic."), CALCULATOR->f_besselj->name());
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::bessely(const Number &o) {
	if(isComplex() || isNegative() || !o.isInteger()) return false;
	if(isZero()) {
		setInfinity(true);
		return true;
	}
	if(isPlusInfinity()) {
		clear(true);
		return true;
	}
	if(isInfinite()) return false;
	if(!mpz_fits_slong_p(mpq_numref(o.internalRational()))) return false;
	long int n = mpz_get_si(mpq_numref(o.internalRational()));
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	if(mpfr_get_exp(fl_value) > 2000000L) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_yn(fl_value, n, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_yn(fl_value, n, fl_value, MPFR_RNDN);
		mpfr_yn(fu_value, n, fu_value, MPFR_RNDN);
		if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
		CALCULATOR->error(false, _("%s() lacks proper support interval arithmetic."), CALCULATOR->f_bessely->name());
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}

bool Number::sin() {
	if(isInfinite()) return false;
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*i_value, false, true);
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.sin() || !t1b.cosh() || !t2a.cos() || !t2b.sinh() || !t1a.multiply(t1b) || !t2a.multiply(t2b)) return false;
			if(!t1a.isReal() || !t2a.isReal()) return false;
			set(t1a, true, true);
			i_value->set(t2a, true, true);
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		} else {
			if(!i_value->sinh()) return false;
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		}
	}
	Number nr_bak(*this);
	bool do_pi = true;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_cmp_ui(mpq_denref(r_value), 1000000L) < 0) do_pi = false;
		if(!setToFloatingPoint()) return false;
	}
	if(mpfr_get_exp(fl_value) > 2000000L) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		if(do_pi) {
			mpfr_t f_pi, f_quo;
			mpz_t f_int;
			mpz_init(f_int);
			mpfr_init2(f_pi, BIT_PRECISION);
			mpfr_init2(f_quo, BIT_PRECISION - 30);
			mpfr_const_pi(f_pi, MPFR_RNDN);
			mpfr_div(f_quo, fl_value, f_pi, MPFR_RNDN);
			mpfr_get_z(f_int, f_quo, MPFR_RNDD);
			mpfr_frac(f_quo, f_quo, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				clear(true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				mpz_clear(f_int);
				return true;
			}
			mpfr_abs(f_quo, f_quo, MPFR_RNDN);
			mpfr_mul_ui(f_quo, f_quo, 2, MPFR_RNDN);
			mpfr_sub_ui(f_quo, f_quo, 1, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				if(mpz_odd_p(f_int)) set(-1, 1, 0, true);
				else set(1, 1, 0, true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				mpz_clear(f_int);
				return true;
			}		
			mpfr_clears(f_pi, f_quo, NULL);
			mpz_clear(f_int);
		}
		mpfr_sin(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_t fl_pi, fu_pi, fu_quo, fl_quo;
		mpfr_inits2(BIT_PRECISION, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpz_t fl_int, fu_int, f_diff;
		mpz_inits(fl_int, fu_int, f_diff, NULL);
		mpfr_const_pi(fl_pi, MPFR_RNDD);
		mpfr_const_pi(fu_pi, MPFR_RNDU);
		mpfr_div(fl_quo, fl_value, fu_pi, MPFR_RNDD);
		mpfr_div(fu_quo, fu_value, fl_pi, MPFR_RNDU);
		mpfr_sub_q(fl_quo, fl_quo, nr_half.internalRational(), MPFR_RNDD);
		mpfr_sub_q(fu_quo, fu_quo, nr_half.internalRational(), MPFR_RNDU);
		mpfr_get_z(fl_int, fl_quo, MPFR_RNDD);
		mpfr_get_z(fu_int, fu_quo, MPFR_RNDD);
		mpfr_sub_z(fl_quo, fl_quo, fl_int, MPFR_RNDD);
		mpfr_sub_z(fu_quo, fu_quo, fu_int, MPFR_RNDU);
		if(mpz_cmp(fl_int, fu_int) != 0) {
			mpz_sub(f_diff, fu_int, fl_int);
			if(mpz_cmp_ui(f_diff, 2) >= 0) {
				mpfr_set_si(fl_value, -1, MPFR_RNDD);
				mpfr_set_si(fu_value, 1, MPFR_RNDU);
			} else {
				if(mpz_even_p(fl_int)) {
					mpfr_sin(fl_value, fl_value, MPFR_RNDU);
					mpfr_sin(fu_value, fu_value, MPFR_RNDU);
					if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
					mpfr_set_si(fl_value, -1, MPFR_RNDD);
				} else {
					mpfr_sin(fl_value, fl_value, MPFR_RNDD);
					mpfr_sin(fu_value, fu_value, MPFR_RNDD);
					if(mpfr_cmp(fu_value, fl_value) < 0) mpfr_swap(fl_value, fu_value);
					mpfr_set_si(fu_value, 1, MPFR_RNDU);
				}
			}
		} else {
			if(mpz_even_p(fl_int)) {
				mpfr_sin(fl_value, fl_value, MPFR_RNDU);
				mpfr_sin(fu_value, fu_value, MPFR_RNDD);
				mpfr_swap(fl_value, fu_value);
			} else {
				mpfr_sin(fl_value, fl_value, MPFR_RNDD);
				mpfr_sin(fu_value, fu_value, MPFR_RNDU);
			}
		}	
		mpfr_clears(fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpz_clears(fl_int, fu_int, f_diff, NULL);
	}
	
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::asin() {
	if(isInfinite()) return false;
	if(isZero()) return true;
	if(isOne()) {
		pi();
		divide(2);
		return true;
	}
	if(isMinusOne()) {
		pi();
		divide(-2);
		return true;
	}
	if(isComplex() || !isFraction()) {
		if(b_imag) return false;
		Number z_sqln(*this);
		Number i_z(*this);
		if(!i_z.multiply(nr_one_i)) return false;
		if(!z_sqln.square() || !z_sqln.negate() || !z_sqln.add(1) || !z_sqln.raise(nr_half) || !z_sqln.add(i_z) || !z_sqln.ln() || !z_sqln.multiply(nr_minus_i)) return false;
		set(z_sqln);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_asin(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_asin(fl_value, fl_value, MPFR_RNDD);
		mpfr_asin(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::sinh() {
	if(isInfinite()) return true;
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*i_value, false, true);
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.sinh() || !t1b.cos() || !t2a.cosh() || !t2b.sin() || !t1a.multiply(t1b) || !t2a.multiply(t2b)) return false;
			if(!t1a.isReal() || !t2a.isReal()) return false;
			set(t1a, true, true);
			i_value->set(t2a, true, true);
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		} else {
			if(!i_value->sin()) return false;
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_sinh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_sinh(fl_value, fl_value, MPFR_RNDD);
		mpfr_sinh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::asinh() {
	if(isInfinite()) return true;
	if(isZero()) return true;
	if(isComplex()) {
		Number z_sqln(*this);
		if(!z_sqln.square() || !z_sqln.add(1) || !z_sqln.raise(nr_half) || !z_sqln.add(*this)) return false;
		//If zero, it means that the precision is too low (since infinity is not the correct value). Happens with number less than -(10^1000)i
		if(z_sqln.isZero()) return false;
		if(!z_sqln.ln()) return false;
		set(z_sqln);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_asinh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_asinh(fl_value, fl_value, MPFR_RNDD);
		mpfr_asinh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::cos() {
	if(isInfinite()) return false;
	if(isZero()) {
		set(1, 1, 0, true);
		return true;
	}
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*i_value, false, true);
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.cos() || !t1b.cosh() || !t2a.sin() || !t2b.sinh() || !t1a.multiply(t1b) || !t2a.multiply(t2b) || !t2a.negate()) return false;
			if(!t1a.isReal() || !t2a.isReal()) return false;
			set(t1a, true, true);
			i_value->set(t2a, true, true);
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		} else {
			if(!i_value->cosh()) return false;
			set(*i_value, true, true);
			i_value->clear();
			return true;
		}
	}
	Number nr_bak(*this);
	bool do_pi = true;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_cmp_ui(mpq_denref(r_value), 1000000L) < 0) do_pi = false;
		if(!setToFloatingPoint()) return false;
	}
	if(mpfr_get_exp(fl_value) > 2000000L) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		if(do_pi) {
			mpfr_t f_pi, f_quo;
			mpz_t f_int;
			mpz_init(f_int);
			mpfr_init2(f_pi, BIT_PRECISION);
			mpfr_init2(f_quo, BIT_PRECISION - 30);
			mpfr_const_pi(f_pi, MPFR_RNDN);
			mpfr_div(f_quo, fl_value, f_pi, MPFR_RNDN);
			// ?: was MPFR_RNDF
			mpfr_get_z(f_int, f_quo, MPFR_RNDD);
			mpfr_frac(f_quo, f_quo, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				if(mpz_odd_p(f_int)) set(-1, 1, 0, true);
				else set(1, 1, 0, true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				mpz_clear(f_int);
				return true;
			}
			mpfr_abs(f_quo, f_quo, MPFR_RNDN);
			mpfr_mul_ui(f_quo, f_quo, 2, MPFR_RNDN);
			mpfr_sub_ui(f_quo, f_quo, 1, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				clear(true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				mpz_clear(f_int);
				return true;
			}
			mpfr_clears(f_pi, f_quo, NULL);
			mpz_clear(f_int);
		}
		mpfr_cos(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_t fl_pi, fu_pi, fu_quo, fl_quo;
		mpfr_inits2(BIT_PRECISION, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpz_t fl_int, fu_int, f_diff;
		mpz_inits(fl_int, fu_int, f_diff, NULL);
		mpfr_const_pi(fl_pi, MPFR_RNDD);
		mpfr_const_pi(fu_pi, MPFR_RNDU);
		mpfr_div(fl_quo, fl_value, fu_pi, MPFR_RNDD);
		mpfr_div(fu_quo, fu_value, fl_pi, MPFR_RNDU);
		mpfr_get_z(fl_int, fl_quo, MPFR_RNDD);
		mpfr_get_z(fu_int, fu_quo, MPFR_RNDD);
		mpfr_sub_z(fl_quo, fl_quo, fl_int, MPFR_RNDD);
		mpfr_sub_z(fu_quo, fu_quo, fu_int, MPFR_RNDU);
		if(mpz_cmp(fl_int, fu_int) != 0) {
			mpz_sub(f_diff, fu_int, fl_int);
			if(mpz_cmp_ui(f_diff, 2) >= 0) {
				mpfr_set_si(fl_value, -1, MPFR_RNDD);
				mpfr_set_si(fu_value, 1, MPFR_RNDU);
			} else {
				if(mpz_even_p(fl_int)) {
					mpfr_cos(fl_value, fl_value, MPFR_RNDU);
					mpfr_cos(fu_value, fu_value, MPFR_RNDU);
					if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
					mpfr_set_si(fl_value, -1, MPFR_RNDD);
				} else {
					mpfr_cos(fl_value, fl_value, MPFR_RNDD);
					mpfr_cos(fu_value, fu_value, MPFR_RNDD);
					if(mpfr_cmp(fu_value, fl_value) < 0) mpfr_swap(fl_value, fu_value);
					mpfr_set_si(fu_value, 1, MPFR_RNDU);
				}
			}
		} else {
			if(mpz_even_p(fl_int)) {
				mpfr_cos(fl_value, fl_value, MPFR_RNDU);
				mpfr_cos(fu_value, fu_value, MPFR_RNDD);
				mpfr_swap(fl_value, fu_value);
			} else {
				mpfr_cos(fl_value, fl_value, MPFR_RNDD);
				mpfr_cos(fu_value, fu_value, MPFR_RNDU);
			}
		}	
		mpfr_clears(fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpz_clears(fl_int, fu_int, f_diff, NULL);
	}
	
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}	
bool Number::acos() {
	if(isInfinite()) return false;
	if(isOne()) {
		clear(true);
		return true;
	}
	if(isZero()) {
		pi();
		divide(2);
		return true;
	}
	if(isMinusOne()) {
		pi();
		return true;
	}
	if(isComplex() || !isFraction()) {
		if(b_imag) return false;
		Number z_sqln(*this);
		if(!z_sqln.square() || !z_sqln.negate() || !z_sqln.add(1) || !z_sqln.raise(nr_half) || !z_sqln.multiply(nr_one_i) || !z_sqln.add(*this) || !z_sqln.ln() || !z_sqln.multiply(nr_minus_i)) return false;
		set(z_sqln);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();

	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_acos(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_acos(fl_value, fl_value, MPFR_RNDU);
		mpfr_acos(fu_value, fu_value, MPFR_RNDD);
		mpfr_swap(fl_value, fu_value);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::cosh() {
	if(isInfinite()) {
		//setInfinity();
		//return true;
		return false;
	}
	if(isZero()) {
		set(1, 1, 0, true);
		return true;
	}
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*i_value, false, true);
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.cosh() || !t1b.cos() || !t2a.sinh() || !t2b.sin() || !t1a.multiply(t1b) || !t2a.multiply(t2b)) return false;
			if(!t1a.isReal() || !t2a.isReal()) return false;
			set(t1a, true, true);
			i_value->set(t2a, true, true);
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		} else {
			if(!i_value->cos()) return false;
			set(*i_value, true, true);
			i_value->clear();
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_cosh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		if(mpfr_sgn(fl_value) < 0) {
			if(mpfr_sgn(fu_value) == 0) {
				mpfr_cosh(fu_value, fl_value, MPFR_RNDU);
				mpfr_set_ui(fl_value, 1, MPFR_RNDD);
			} else if(mpfr_sgn(fu_value) > 0) {
				mpfr_cosh(fl_value, fl_value, MPFR_RNDU);
				mpfr_cosh(fu_value, fu_value, MPFR_RNDD);
				if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
				mpfr_set_ui(fl_value, 1, MPFR_RNDD);
			} else {
				mpfr_cosh(fl_value, fl_value, MPFR_RNDU);
				mpfr_cosh(fu_value, fu_value, MPFR_RNDD);
				mpfr_swap(fl_value, fu_value);
			}
		} else {
			mpfr_cosh(fu_value, fu_value, MPFR_RNDU);
			if(mpfr_sgn(fl_value) == 0) mpfr_set_ui(fl_value, 1, MPFR_RNDD);
			else mpfr_cosh(fl_value, fl_value, MPFR_RNDD);
		}
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::acosh() {
	if(isPlusInfinity() || isInfinity()) return true;
	if(isMinusInfinity()) return false;
	if(isOne()) {
		clear(true);
		return true;
	}
	if(isComplex() || !isGreaterThanOrEqualTo(nr_one)) {
		if(b_imag) return false;
		Number ipz(*this), imz(*this);
		if(!ipz.add(1) || !imz.subtract(1)) return false;
		if(!ipz.raise(nr_half) || !imz.raise(nr_half) || !ipz.multiply(imz) || !ipz.add(*this) || !ipz.ln()) return false;
		set(ipz);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_acosh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_acosh(fl_value, fl_value, MPFR_RNDD);
		mpfr_acosh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::tan() {
	if(isInfinite()) return false;
	if(isZero()) return true;
	if(isComplex()) {
		Number num(*this), den(*this);
		if(!num.sin() || !den.cos() || !num.divide(den)) return false;
		set(num, true);
		return true;
	}
	Number nr_bak(*this);
	bool do_pi = true;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_cmp_ui(mpq_denref(r_value), 1000000L) < 0) do_pi = false;
		if(!setToFloatingPoint()) return false;
	}
	if(mpfr_get_exp(fl_value) > 2000000L) {
		set(nr_bak);
		return false;
	}
	
	mpfr_clear_flags();
	
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		if(do_pi) {
			mpfr_t f_pi, f_quo;
			mpfr_init2(f_pi, BIT_PRECISION);
			mpfr_init2(f_quo, BIT_PRECISION - 30);
			mpfr_const_pi(f_pi, MPFR_RNDN);
			mpfr_div(f_quo, fl_value, f_pi, MPFR_RNDN);
			mpfr_frac(f_quo, f_quo, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				clear(true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				return true;
			}
			mpfr_clears(f_pi, f_quo, NULL);
		}
		mpfr_tan(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_t fl_pi, fu_pi, fu_quo, fl_quo, f_diff;
		mpfr_inits2(BIT_PRECISION, fl_pi, fu_pi, f_diff, fl_quo, fu_quo, NULL);
		mpfr_const_pi(fl_pi, MPFR_RNDD);
		mpfr_const_pi(fu_pi, MPFR_RNDU);
		mpfr_div(fl_quo, fl_value, fu_pi, MPFR_RNDD);
		mpfr_div(fu_quo, fu_value, fl_pi, MPFR_RNDU);
		mpfr_sub(f_diff, fu_quo, fl_quo, MPFR_RNDU);
		if(mpfr_cmp_ui(f_diff, 1) >= 0) {
			mpfr_clears(f_diff, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
			set(nr_bak);
			return false;
		}
		mpfr_frac(fl_quo, fl_quo, MPFR_RNDD);
		mpfr_frac(fu_quo, fu_quo, MPFR_RNDU);
		int c1 = mpfr_cmp_ui_2exp(fl_quo, 1, -1);
		int c2 = mpfr_cmp_ui_2exp(fu_quo, 1, -1);
		if((c1 != c2 && c1 <= 0 && c2 >= 0) || (c1 == c2 && mpfr_cmp_ui_2exp(f_diff, 1, -1) >= 0)) {
			mpfr_clears(f_diff, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
			set(nr_bak);
			return false;
		}
		mpfr_clears(f_diff, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpfr_tan(fl_value, fl_value, MPFR_RNDD);
		mpfr_tan(fu_value, fu_value, MPFR_RNDU);
	}

	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::atan() {
	if(isInfinity()) return false;
	if(isZero()) return true;
	if(isInfinite()) {
		pi();
		divide(2);
		if(isMinusInfinity()) negate();
		return true;
	}
	if(isComplex()) {
		if(b_imag) return false;
		Number ipz(nr_one_i), imz(nr_one_i);
		if(!ipz.add(*this) || !imz.subtract(*this)) return false;
		if(!ipz.divide(imz) || !ipz.ln() || !ipz.multiply(nr_one_i) || !ipz.divide(2)) return false;
		set(ipz);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_atan(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_atan(fl_value, fl_value, MPFR_RNDD);
		mpfr_atan(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::atan2(const Number &o) {
	if(isInfinite() || o.isInfinite()) return false;
	if(isComplex() || o.isComplex()) return false;
	if(isZero()) {
		if(o.isZero()) return false;
		if(o.isPositive()) {
			clear();
			setPrecisionAndApproximateFrom(o);
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(o.isFloatingPoint()) {
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval() && !o.isInterval()) {
			mpfr_atan2(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			if(!o.isNonZero() && (mpfr_sgn(fl_value) != mpfr_sgn(fu_value) || mpfr_sgn(fl_value) == 0)) {set(nr_bak); return false;}
			mpfr_atan2(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
			mpfr_atan2(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDU);
		}
	} else {
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_t of_value;
			mpfr_init2(of_value, BIT_PRECISION);
			mpfr_set_q(of_value, o.internalRational(), MPFR_RNDN);
			mpfr_atan2(fl_value, fl_value, of_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
			mpfr_clear(of_value);
		} else {
			mpfr_t ofl_value, ofu_value;
			mpfr_inits2(BIT_PRECISION, ofl_value, ofu_value, NULL);
			mpfr_set_q(ofl_value, o.internalRational(), MPFR_RNDD);
			mpfr_set_q(ofu_value, o.internalRational(), MPFR_RNDU);
			mpfr_atan2(fl_value, fl_value, ofl_value, MPFR_RNDD);
			mpfr_atan2(fu_value, fu_value, ofu_value, MPFR_RNDU);
			mpfr_clears(ofl_value, ofu_value, NULL);
		}
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::arg() {
	if(isInfinite()) return false;
	if(isZero()) return false;
	if(!isComplex()) {
		if(isNegative()) {
			pi();
		} else {
			clear(true);
		}
		return true;
	}
	if(!hasRealPart()) {
		bool b_neg = i_value->isNegative();
		pi();
		multiply(nr_half);
		if(b_neg) negate();
		return true;
	}
	Number *i_value2 = i_value;
	i_value = NULL;
	if(!i_value2->atan2(*this)) {
		i_value = i_value2;
		return false;
	}
	set(*i_value2);
	delete i_value2;
	return true;
}
bool Number::tanh() {
	if(isInfinity()) return true;
	if(isPlusInfinity()) set(1);
	if(isMinusInfinity()) set(-1);
	if(isZero()) return true;
	if(isComplex()) {
		Number num(*this), den(*this);
		if(!num.sinh() || !den.cosh() || !num.divide(den)) return false;
		set(num, true);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_tanh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_tanh(fl_value, fl_value, MPFR_RNDD);
		mpfr_tanh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::atanh() {
	if(isInfinite()) return false;
	if(isZero()) return true;
	if(isOne()) {
		if(b_imag) return false;
		setPlusInfinity();
		return true;
	}
	if(isMinusOne()) {
		if(b_imag) return false;
		setMinusInfinity();
		return true;
	}
	if(isComplex() || !isFraction()) {
		if(b_imag) return false;
		Number ipz(nr_one), imz(nr_one);
		if(!ipz.add(*this) || !imz.subtract(*this) || !ipz.divide(imz) || !ipz.ln() || !ipz.divide(2)) return false;
		set(ipz);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_atanh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_atanh(fl_value, fl_value, MPFR_RNDD);
		mpfr_atanh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::ln() {
	if(isPlusInfinity()) return true;
	if(isInfinite()) return false;
	if(isOne() && !isApproximate()) {
		clear();
		return true;
	}

	if(isZero()) {
		if(b_imag) return false;
		setMinusInfinity();
		return true;
	}

	if(!isNonZero()) return false;

	if(hasImaginaryPart()) {
		Number new_i(*i_value);
		Number new_r(*this);
		Number this_r;
		this_r.set(*this, false, true);
		if(!new_i.atan2(this_r) || !new_r.abs() || new_i.isComplex()) return false;
		if(new_r.isComplex() || !new_r.ln()) return false;
		set(new_r);
		setImaginaryPart(new_i);
		return true;
	} else if(isNegative()) {
		if(b_imag) return false;
		Number new_i;
		new_i.markAsImaginaryPart();
		new_i.pi();
		Number new_r(*this);
		if(!new_r.abs() || !new_r.ln()) return false;
		set(new_r);
		setImaginaryPart(new_i);
		return true;
	}

	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
		
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_log(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_log(fl_value, fl_value, MPFR_RNDD);
		mpfr_log(fu_value, fu_value, MPFR_RNDU);
	}

	if(!testFloatResult(false)) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::log(const Number &o) {
	if(isPlusInfinity()) return true;
	if(isInfinite()) return false;
	if(isOne()) {
		bool was_approx = b_approx || o.isApproximate();
		clear();
		b_approx = was_approx;
		return true;
	}
	if(isZero()) {
		if(b_imag) return false;
		bool was_approx = b_approx || o.isApproximate();
		setMinusInfinity();
		b_approx = was_approx;
		return true;
	}
	if(!isNonZero()) return false;
	if(o.isZero()) {
		clear();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(!o.isNonZero()) return false;
	if(o.isOne()) {
		//setInfinity();
		//return true;
		return false;
	}
	if(isComplex() || o.isComplex() || o.isNegative() || isNegative()) {
		Number num(*this);
		Number den(o);
		if(!num.ln() || !den.ln() || !den.recip() || !num.multiply(den)) return false;
		if(b_imag && num.isComplex()) return false;
		set(num);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(o.isRational() && o == 2) {
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_log2(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_log2(fl_value, fl_value, MPFR_RNDD);
			mpfr_log2(fu_value, fu_value, MPFR_RNDU);
		}
	} else if(o.isRational() && o == 10) {
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			mpfr_log10(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_log10(fl_value, fl_value, MPFR_RNDD);
			mpfr_log10(fu_value, fu_value, MPFR_RNDU);
		}
	} else {
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval() && !o.isInterval()) {
			mpfr_t fl_base;
			mpfr_inits2(BIT_PRECISION, fl_base, NULL);
			if(o.isRational()) mpfr_set_q(fl_base, o.internalRational(), MPFR_RNDN);
			else mpfr_set(fl_base, o.internalLowerFloat(), MPFR_RNDN);
			mpfr_log(fl_value, fl_value, MPFR_RNDN);
			mpfr_log(fl_base, fl_base, MPFR_RNDN);
			mpfr_div(fl_value, fl_value, fl_base, MPFR_RNDN);
			mpfr_clear(fl_base);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_t fl_base, fu_base;
			mpfr_inits2(BIT_PRECISION, fl_base, fu_base, NULL);
			if(o.isRational()) {
				mpfr_set_q(fl_base, o.internalRational(), MPFR_RNDD);
				mpfr_set_q(fu_base, o.internalRational(), MPFR_RNDU);
			} else {
				mpfr_set(fl_base, o.internalLowerFloat(), MPFR_RNDD);
				mpfr_set(fu_base, o.internalUpperFloat(), MPFR_RNDU);
			}
			mpfr_log(fl_value, fl_value, MPFR_RNDD);
			mpfr_log(fu_value, fu_value, MPFR_RNDU);
			mpfr_log(fl_base, fl_base, MPFR_RNDD);
			mpfr_log(fu_base, fu_base, MPFR_RNDU);
			mpfr_div(fl_value, fl_value, fu_base, MPFR_RNDD);
			mpfr_div(fu_value, fu_value, fl_base, MPFR_RNDU);
			mpfr_clears(fl_base, fu_base, NULL);
		}
	}
	if(!testFloatResult(false)) {
		set(nr_bak);
		return false;
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::exp() {
	if(isInfinity()) return false;
	if(isPlusInfinity()) return true;
	if(isMinusInfinity()) {
		clear();
		return true;
	}
	if(isComplex()) {
		Number e_base;
		e_base.e();
		if(!e_base.raise(*this)) return false;
		set(e_base);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
		mpfr_exp(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_exp(fu_value, fu_value, MPFR_RNDU);
		mpfr_exp(fl_value, fl_value, MPFR_RNDD);
	}
	if(!testFloatResult(false)) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::lambertW() {

	if(!isReal()) return false;
	if(isZero()) return true;
	
	Number nr_bak(*this);
	mpfr_clear_flags();
	
	for(size_t i = 0; i < 2 && (CALCULATOR->usesIntervalArithmetics() || isInterval() || i < 1); i++) {
		mpfr_t x, m1_div_exp1;
		mpfr_inits2(BIT_PRECISION, x, m1_div_exp1, NULL);
		if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
			if(n_type != NUMBER_TYPE_FLOAT) mpfr_set_q(x, r_value, MPFR_RNDN);
			else mpfr_set(x, fl_value, MPFR_RNDN);
		} else if(i == 0) {
			if(n_type != NUMBER_TYPE_FLOAT) mpfr_set_q(x, r_value, MPFR_RNDD);
			else mpfr_set(x, fl_value, MPFR_RNDD);
		} else if(i == 1) {
			if(n_type != NUMBER_TYPE_FLOAT) mpfr_set_q(x, r_value, MPFR_RNDU);
			else mpfr_set(x, fu_value, MPFR_RNDU);
		}
		mpfr_set_ui(m1_div_exp1, 1, MPFR_RNDN);
		mpfr_exp(m1_div_exp1, m1_div_exp1, MPFR_RNDN);
		mpfr_ui_div(m1_div_exp1, 1, m1_div_exp1, MPFR_RNDN);
		mpfr_neg(m1_div_exp1, m1_div_exp1, MPFR_RNDN);
		int cmp = mpfr_cmp(x, m1_div_exp1);
		if(cmp == 0) {
			mpfr_clears(x, m1_div_exp1, NULL);
			if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) {
				set(-1, 1);
				b_approx = true;
				i_precision = PRECISION;
				return true;
			} else if(i == 0) {
				mpfr_set_ui(fl_value, -1, MPFR_RNDD);
			} else {
				mpfr_set_ui(fu_value, -1, MPFR_RNDU);
			}
		} else if(cmp < 0) {
			mpfr_clears(x, m1_div_exp1, NULL);
			set(nr_bak);
			return false;
		} else {
			mpfr_t w;
			mpfr_init2(w, BIT_PRECISION);
			mpfr_set_zero(w, 0);
			cmp = mpfr_cmp_ui(x, 10);
			if(cmp > 0) {
				mpfr_log(w, x, MPFR_RNDN);
				mpfr_t wln;
				mpfr_init2(wln, BIT_PRECISION);
				mpfr_log(wln, w, MPFR_RNDN);
				mpfr_sub(w, w, wln, MPFR_RNDN);
				mpfr_clear(wln);
			}
			
			mpfr_t wPrec, wTimesExpW, wPlusOneTimesExpW, testXW, tmp1, tmp2;
			mpfr_inits2(BIT_PRECISION, wPrec, wTimesExpW, wPlusOneTimesExpW, testXW, tmp1, tmp2, NULL);
			mpfr_set_si(wPrec, -(BIT_PRECISION - 30), MPFR_RNDN);
			mpfr_exp2(wPrec, wPrec, MPFR_RNDN);
			while(true) {
				if(CALCULATOR->aborted() || testErrors()) {
					mpfr_clears(x, m1_div_exp1, w, wPrec, wTimesExpW, wPlusOneTimesExpW, testXW, tmp1, tmp2, NULL);
					set(nr_bak);
					return false;
				}
				mpfr_exp(wTimesExpW, w, MPFR_RNDN);
				mpfr_set(wPlusOneTimesExpW, wTimesExpW, MPFR_RNDN);
				mpfr_mul(wTimesExpW, wTimesExpW, w, MPFR_RNDN);
				mpfr_add(wPlusOneTimesExpW, wPlusOneTimesExpW, wTimesExpW, MPFR_RNDN);
				mpfr_sub(testXW, x, wTimesExpW, MPFR_RNDN);
				mpfr_div(testXW, testXW, wPlusOneTimesExpW, MPFR_RNDN);
				mpfr_abs(testXW, testXW, MPFR_RNDN);
				if(mpfr_cmp(wPrec, testXW) > 0) {
					break;
				}
				mpfr_sub(wTimesExpW, wTimesExpW, x, MPFR_RNDN);
				mpfr_add_ui(tmp1, w, 2, MPFR_RNDN);
				mpfr_mul(tmp2, wTimesExpW, tmp1, MPFR_RNDN);
				mpfr_mul_ui(tmp1, w, 2, MPFR_RNDN);
				mpfr_add_ui(tmp1, tmp1, 2, MPFR_RNDN);
				mpfr_div(tmp2, tmp2, tmp1, MPFR_RNDN);
				mpfr_sub(wPlusOneTimesExpW, wPlusOneTimesExpW, tmp2, MPFR_RNDN);
				mpfr_div(wTimesExpW, wTimesExpW, wPlusOneTimesExpW, MPFR_RNDN);
				mpfr_sub(w, w, wTimesExpW, MPFR_RNDN);
			}
			if(n_type == NUMBER_TYPE_RATIONAL) {
				mpfr_init2(fl_value, BIT_PRECISION);
				n_type = NUMBER_TYPE_FLOAT;
				mpq_set_ui(r_value, 0, 1);
			}
			if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) mpfr_set(fl_value, w, MPFR_RNDN);
			else if(i == 0) mpfr_set(fl_value, w, MPFR_RNDD);
			else if(i == 1) mpfr_set(fu_value, w, MPFR_RNDU);
			mpfr_clears(x, m1_div_exp1, w, wPrec, wTimesExpW, wPlusOneTimesExpW, testXW, tmp1, tmp2, NULL);
			if(i_precision < 0 || i_precision > PRECISION) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
		}
	}
	if(!CALCULATOR->usesIntervalArithmetics() && !isInterval()) mpfr_set(fu_value, fl_value, MPFR_RNDN);
	if(!testFloatResult(false)) {
		set(nr_bak);
		return false;
	}
	b_approx = true;
	return true;

}
bool Number::gcd(const Number &o) {
	if(!isInteger() || !o.isInteger()) {
		return false;
	}
	if(isZero() && o.isZero()) {
		clear(); 
		return true;
	}
	mpz_gcd(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::lcm(const Number &o) {
	if(isInteger() && o.isInteger()) {
		mpz_lcm(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
		return true;
	}
	return multiply(o);
}

bool recfact(mpz_ptr ret, long int start, long int n) {
	long int i;
	if(n <= 16) { 
		mpz_set_si(ret, start);
		for(i = start + 1; i < start + n; i++) mpz_mul_si(ret, ret, i);
		return true;
	}
	if(CALCULATOR->aborted()) return false;
	i = n / 2;
	if(!recfact(ret, start, i)) return false;
	mpz_t retmul;
	mpz_init(retmul);
	if(!recfact(retmul, start + i, n - i)) return false;
	mpz_mul(ret, ret, retmul);
	mpz_clear(retmul);
	return true;
}
bool recfact2(mpz_ptr ret, long int start, long int n) {
	long int i;
	if(n <= 32) { 
		mpz_set_si(ret, start + n - 1);
		for(i = start + n - 3; i >= start; i -= 2) mpz_mul_si(ret, ret, i);
		return true;
	}
	if(CALCULATOR->aborted()) return false;
	i = n / 2;
	if(n % 2 != i % 2) i--;
	if(!recfact2(ret, start, i)) return false;
	mpz_t retmul;
	mpz_init(retmul);
	if(!recfact2(retmul, start + i, n - i)) return false;
	mpz_mul(ret, ret, retmul);
	mpz_clear(retmul);
	return true;
}
bool recfactm(mpz_ptr ret, long int start, long int n, long int m) {
	long int i;
	if(n <= 16 * m) { 
		mpz_set_si(ret, start + n - 1);
		for(i = start + n - 1 - m; i >= start; i -= m) mpz_mul_si(ret, ret, i);
		return true;
	}
	if(CALCULATOR->aborted()) return false;
	i = n / 2;
	i -= ((i % m) - (n % m));
	if(!recfactm(ret, start, i, m)) return false;
	mpz_t retmul;
	mpz_init(retmul);
	if(!recfactm(retmul, start + i, n - i, m)) return false;
	mpz_mul(ret, ret, retmul);
	mpz_clear(retmul);
	return true;
}

bool Number::factorial() {
	if(!isInteger()) {
		return false;
	}
	if(isNegative()) {
		/*if(b_imag) return false;
		setPlusInfinity();
		return true;*/
		return false;
	}
	if(isZero()) {
		set(1);
		return true;
	} else if(isOne()) {
		return true;
	} else if(isNegative()) {
		return false;
	}
	if(!mpz_fits_slong_p(mpq_numref(r_value))) return false;
	long int n = mpz_get_si(mpq_numref(r_value));
	if(!recfact(mpq_numref(r_value), 1, n)) {
		mpz_set_si(mpq_numref(r_value), n);
		return false;
	}
	return true;
}
bool Number::multiFactorial(const Number &o) {
	if(!isInteger() || !o.isInteger() || !o.isPositive()) {
		return false;
	}
	if(isZero()) {
		set(1, 1);
		return true;
	} else if(isOne()) {
		return true;
	} else if(isNegative()) {
		return false;
	}
	if(!mpz_fits_slong_p(mpq_numref(r_value)) || !mpz_fits_slong_p(mpq_numref(o.internalRational()))) return false;
	long int n = mpz_get_si(mpq_numref(r_value));
	long int m = mpz_get_si(mpq_numref(o.internalRational()));
	if(!recfactm(mpq_numref(r_value), 1, n, m)) {
		mpz_set_si(mpq_numref(r_value), n);
		return false;
	}
	return true;
}
bool Number::doubleFactorial() {
	if(!isInteger()) {
		return false;
	}
	if(isZero() || isMinusOne()) {
		set(1, 1);
		return true;
	} else if(isOne()) {
		return true;
	} else if(isNegative()) {
		return false;
	}
	if(!mpz_fits_slong_p(mpq_numref(r_value))) return false;
	unsigned long int n = mpz_get_si(mpq_numref(r_value));
	if(!recfact2(mpq_numref(r_value), 1, n)) {
		mpz_set_si(mpq_numref(r_value), n);
		return false;
	}
	return true;
}
bool Number::binomial(const Number &m, const Number &k) {
	if(!m.isInteger() || !k.isInteger()) return false;
	if(k.isNegative()) return false;
	if(m.isZero() || m.isNegative()) return false;
	if(k.isGreaterThan(m)) return false;
	if(!mpz_fits_ulong_p(mpq_numref(k.internalRational()))) return false;
	mpz_bin_ui(mpq_numref(r_value), mpq_numref(m.internalRational()), mpz_get_ui(mpq_numref(k.internalRational())));
	return true;
}

bool Number::factorize(vector<Number> &factors) {
	if(isZero() || !isInteger()) return false;
	if(mpz_cmp_si(mpq_numref(r_value), 1) == 0) {
		factors.push_back(nr_one);
		return true;
	}
	if(mpz_cmp_si(mpq_numref(r_value), -1) == 0) {
		factors.push_back(nr_minus_one);
		return true;
	}
	mpz_t inr, last_prime, facmax;
	mpz_inits(inr, last_prime, facmax, NULL);
	mpz_set(inr, mpq_numref(r_value));
	if(mpz_sgn(inr) < 0) {
		mpz_neg(inr, inr);
		factors.push_back(nr_minus_one);
	}
	size_t prime_index = 0;
	bool b = true;
	while(b) {
		if(CALCULATOR->aborted()) {mpz_clears(inr, last_prime, facmax, NULL); return false;}
		b = false;
		mpz_sqrt(facmax, inr);
		for(; prime_index < NR_OF_PRIMES && mpz_cmp_si(facmax, PRIMES[prime_index]) >= 0; prime_index++) {
			if(mpz_divisible_ui_p(inr, (unsigned long int) PRIMES[prime_index])) {
				mpz_divexact_ui(inr, inr, (unsigned long int) PRIMES[prime_index]);
				Number fac(PRIMES[prime_index], 1);;
				factors.push_back(fac);
				b = true;
				break;
			}
		}
		if(prime_index == NR_OF_PRIMES) {
			mpz_set_si(last_prime, PRIMES[NR_OF_PRIMES - 1] + 2);
			prime_index++;
		}
		if(!b && prime_index > NR_OF_PRIMES) {
			while(!b && mpz_cmp(facmax, last_prime) >= 0) {
				if(CALCULATOR->aborted()) {mpz_clears(inr, last_prime, facmax, NULL); return false;}
				if(mpz_divisible_p(inr, last_prime)) {
					mpz_divexact(inr, inr, last_prime);
					b = true;
					Number fac;
					fac.setInternal(last_prime);
					factors.push_back(fac);
					break;
				}
				mpz_add_ui(last_prime, last_prime, 2);
			}
		}
	}
	if(mpz_cmp_si(mpq_numref(r_value), 1) != 0) {
		Number fac;
		fac.setInternal(inr);
		factors.push_back(fac);
	}
	mpz_clears(inr, last_prime, facmax, NULL);
	return true;
}

void Number::rand() {
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_inits2(BIT_PRECISION, fl_value, fu_value, NULL);
		mpq_set_ui(r_value, 0, 1);
		n_type = NUMBER_TYPE_FLOAT;
	}
	mpfr_urandom(fu_value, randstate, MPFR_RNDN);
	mpfr_set(fl_value, fu_value, MPFR_RNDN);
	b_approx = false;
	i_precision = -1;
}
void Number::intRand(const Number &ceil) {
	clear();
	if(!ceil.isInteger() || !ceil.isPositive()) return;
	mpz_urandomm(mpq_numref(r_value), randstate, mpq_numref(ceil.internalRational()));
}

bool Number::add(const Number &o, MathOperation op) {
	switch(op) {
		case OPERATION_SUBTRACT: {
			return subtract(o);
		}
		case OPERATION_ADD: {
			return add(o);
		} 
		case OPERATION_MULTIPLY: {
			return multiply(o);
		}
		case OPERATION_DIVIDE: {
			return divide(o);
		}		
		case OPERATION_RAISE: {
			return raise(o);
		}
		case OPERATION_EXP10: {
			return exp10(o);
		}
		case OPERATION_BITWISE_AND: {
			return bitAnd(o);
		}
		case OPERATION_BITWISE_OR: {
			return bitOr(o);
		}
		case OPERATION_BITWISE_XOR: {
			return bitXor(o);
		}
		case OPERATION_LOGICAL_OR: {
			Number nr;
			ComparisonResult i1 = compare(nr);
			ComparisonResult i2 = o.compare(nr);
			if(i1 == COMPARISON_RESULT_UNKNOWN || i1 == COMPARISON_RESULT_EQUAL_OR_LESS || i1 == COMPARISON_RESULT_NOT_EQUAL) i1 = COMPARISON_RESULT_UNKNOWN;
			if(i2 == COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_EQUAL_OR_LESS || i2 == COMPARISON_RESULT_NOT_EQUAL) i2 = COMPARISON_RESULT_UNKNOWN;
			if(i1 == COMPARISON_RESULT_UNKNOWN && (i2 == COMPARISON_RESULT_UNKNOWN || i2 != COMPARISON_RESULT_LESS)) return false;
			if(i2 == COMPARISON_RESULT_UNKNOWN && (i1 != COMPARISON_RESULT_LESS)) return false;
			setTrue(i1 == COMPARISON_RESULT_LESS || i2 == COMPARISON_RESULT_LESS);
			return true;
		}
		case OPERATION_LOGICAL_XOR: {
			Number nr;
			ComparisonResult i1 = compare(nr);
			ComparisonResult i2 = o.compare(nr);
			if(i1 == COMPARISON_RESULT_UNKNOWN || i1 == COMPARISON_RESULT_EQUAL_OR_LESS || i1 == COMPARISON_RESULT_NOT_EQUAL) return false;
			if(i2 == COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_EQUAL_OR_LESS || i2 == COMPARISON_RESULT_NOT_EQUAL) return false;
			if(i1 == COMPARISON_RESULT_LESS) setTrue(i2 != COMPARISON_RESULT_LESS);
			else setTrue(i2 == COMPARISON_RESULT_LESS);
			return true;
		}
		case OPERATION_LOGICAL_AND: {
			Number nr;
			ComparisonResult i1 = compare(nr);
			ComparisonResult i2 = o.compare(nr);
			if(i1 == COMPARISON_RESULT_UNKNOWN || i1 == COMPARISON_RESULT_EQUAL_OR_LESS || i1 == COMPARISON_RESULT_NOT_EQUAL) i1 = COMPARISON_RESULT_UNKNOWN;
			if(i2 == COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_EQUAL_OR_LESS || i2 == COMPARISON_RESULT_NOT_EQUAL) i2 = COMPARISON_RESULT_UNKNOWN;
			if(i1 == COMPARISON_RESULT_UNKNOWN && (i2 == COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_LESS)) return false;
			if(i2 == COMPARISON_RESULT_UNKNOWN && (i1 == COMPARISON_RESULT_LESS)) return false;
			setTrue(i1 == COMPARISON_RESULT_LESS && i2 == COMPARISON_RESULT_LESS);
			return true;
		}
		case OPERATION_EQUALS: {
			ComparisonResult i = compare(o);
			if(i == COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_GREATER || i == COMPARISON_RESULT_EQUAL_OR_LESS) return false;
			setTrue(i == COMPARISON_RESULT_EQUAL);
			return true;
		}
		case OPERATION_GREATER: {
			ComparisonResult i = compare(o);
			switch(i) {
				case COMPARISON_RESULT_LESS: {
					setTrue();
					return true;
				}
				case COMPARISON_RESULT_GREATER: {}
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {}
				case COMPARISON_RESULT_EQUAL: {
					setFalse();
					return true;
				}
				default: {
					return false;
				}
			}
		}
		case OPERATION_LESS: {
			ComparisonResult i = compare(o);
			switch(i) {
				case COMPARISON_RESULT_GREATER: {
					setTrue();
					return true;
				}
				case COMPARISON_RESULT_LESS: {}
				case COMPARISON_RESULT_EQUAL_OR_LESS: {}
				case COMPARISON_RESULT_EQUAL: {
					setFalse();
					return true;
				}
				default: {
					return false;
				}
			}
		}
		case OPERATION_EQUALS_GREATER: {
			ComparisonResult i = compare(o);
			switch(i) {
				case COMPARISON_RESULT_EQUAL_OR_LESS: {}
				case COMPARISON_RESULT_EQUAL: {}
				case COMPARISON_RESULT_LESS: {
					setTrue();
					return true;
				}
				case COMPARISON_RESULT_GREATER: {
					setFalse();
					return true;
				}
				default: {
					return false;
				}
			}
			return false;
		}
		case OPERATION_EQUALS_LESS: {
			ComparisonResult i = compare(o);
			switch(i) {
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {}
				case COMPARISON_RESULT_EQUAL: {}
				case COMPARISON_RESULT_GREATER: {
					setTrue();
					return true;
				}
				case COMPARISON_RESULT_LESS: {
					setFalse();
					return true;
				}
				default: {
					return false;
				}
			}
			return false;
		}
		case OPERATION_NOT_EQUALS: {
			ComparisonResult i = compare(o);
			if(i == COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_GREATER || i == COMPARISON_RESULT_EQUAL_OR_LESS) return false;
			setTrue(i == COMPARISON_RESULT_NOT_EQUAL || i == COMPARISON_RESULT_GREATER || i == COMPARISON_RESULT_LESS);
			return true;
		}
	}
	return false;	
}
string Number::printNumerator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printMPZ(mpq_numref(r_value), base, display_sign, base_display, lower_case);
}
string Number::printDenominator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printMPZ(mpq_denref(r_value), base, display_sign, base_display, lower_case);
}
string Number::printImaginaryNumerator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printMPZ(mpq_numref(r_value), base, display_sign, base_display, lower_case);
}
string Number::printImaginaryDenominator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printMPZ(mpq_denref(r_value), base, display_sign, base_display, lower_case);
}

int char2val(const char &c, const int &base) {
	if(c <= '9') return c - '0';
	if(base == 12 && c == 'X') return 10;
	else if(base == 12 && c == 'E') return 11;
	else return c - 'A' + 10;
}

string Number::print(const PrintOptions &po, const InternalPrintStruct &ips) const {
	if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
	if(ips.minus) *ips.minus = false;
	if(ips.exp_minus) *ips.exp_minus = false;
	if(ips.num) *ips.num = "";
	if(ips.den) *ips.den = "";
	if(ips.exp) *ips.exp = "";
	if(ips.re) *ips.re = "";
	if(ips.im) *ips.im = "";
	if(ips.iexp) *ips.iexp = 0;
	if(po.is_approximate && isApproximate()) *po.is_approximate = true;
	if((po.base == BASE_SEXAGESIMAL || po.base == BASE_TIME) && isReal()) {
		Number nr(*this);
		bool neg = nr.isNegative();
		nr.setNegative(false);
		nr.trunc();
		PrintOptions po2 = po;
		po2.base = 10;
		po2.number_fraction_format = FRACTION_FRACTIONAL;
		string str = nr.print(po2);
		if(po.base == BASE_SEXAGESIMAL) {
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DEGREE, po.can_display_unicode_string_arg))) {
				str += SIGN_DEGREE;
			} else {
				str += "o";
			}	
		}
		nr = *this;
		nr.frac();
		nr *= 60;
		Number nr2(nr);
		nr.trunc();
		if(po.base == BASE_TIME) {
			str += ":";
			if(nr.isLessThan(10)) {
				str += "0";
			}
		}
		str += nr.printNumerator(10, false);
		if(po.base == BASE_SEXAGESIMAL) {
			str += "'";
		}	
		nr2.frac();
		if(!nr2.isZero() || po.base == BASE_SEXAGESIMAL) {
			nr2.multiply(60);
			nr = nr2;
			nr.trunc();
			nr2.frac();
			if(!nr2.isZero()) {
				if(po.is_approximate) *po.is_approximate = true;
				if(nr2.isGreaterThanOrEqualTo(nr_half)) {
					nr.add(1);
				}
			}
			if(po.base == BASE_TIME) {
				str += ":";
				if(nr.isLessThan(10)) {
					str += "0";
				}
			}
			str += nr.printNumerator(10, false);
			if(po.base == BASE_SEXAGESIMAL) {
				str += "\"";
			}
		}
		if(ips.minus) {
			*ips.minus = neg;
		} else if(neg) {
			str.insert(0, "-");
		}
		if(ips.num) *ips.num = str;
		
		return str;
	}
	string str;
	int base;
	long int min_decimals = 0;
	if(po.use_min_decimals && po.min_decimals > 0) min_decimals = po.min_decimals;
	if((int) min_decimals > po.max_decimals && po.use_max_decimals && po.max_decimals >= 0) {
		min_decimals = po.max_decimals;
	}
	if(po.base <= 1 && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_TIME) base = 10;
	else if(po.base > 36 && po.base != BASE_SEXAGESIMAL) base = 36;
	else base = po.base;
	if(po.base == BASE_ROMAN_NUMERALS) {
		if(!isRational()) {
			CALCULATOR->error(false, _("Can only display rational numbers as roman numerals."), NULL);
			base = 10;
		} else if(mpz_cmpabs_ui(mpq_numref(r_value), 9999) > 0 || mpz_cmp_ui(mpq_denref(r_value), 9999) > 0) {
			CALCULATOR->error(false, _("Cannot display numbers greater than 9999 or less than -9999 as roman numerals."), NULL);
			base = 10;
		}
	}
	
	if(isComplex()) {
		if(!i_value->isNonZero()) {
			Number nr;
			nr.set(*this, false, true);
			return nr.print(po, ips);
		}
		bool bre = hasRealPart();
		if(bre) {
			Number r_nr(*this);
			r_nr.clearImaginary();
			str = r_nr.print(po, ips);
			if(ips.re) *ips.re = str;
			InternalPrintStruct ips_n = ips;
			bool neg = false;
			ips_n.minus = &neg;
			string str2 = i_value->print(po, ips_n);
			if(ips.im) *ips.im = str2;
			if(!po.short_multiplication && str2 != "1") {
				if(po.spacious) {
					str2 += " * ";
				} else {
					str2 += "*";
				}
			}
			if(str2 == "1") str2 = "i";
			else str2 += "i";
			if(*ips_n.minus) {
				str += " - ";
			} else {
				str += " + ";
			}
			str += str2;	
		} else {
			str = i_value->print(po, ips);
			if(ips.im) *ips.im = str;
			if(!po.short_multiplication && str != "1") {
				if(po.spacious) {
					str += " * ";
				} else {
					str += "*";
				}
			}
			if(str == "1") str = "i";
			else str += "i";
		}
		if(ips.num) *ips.num = str;
		return str;
	}

	long int precision = PRECISION;
	if(b_approx && i_precision >= 0 && (po.preserve_precision || i_precision < PRECISION)) precision = i_precision;
	if(b_approx && i_precision < 0 && po.preserve_precision && FROM_BIT_PRECISION(BIT_PRECISION) > precision) precision = FROM_BIT_PRECISION(BIT_PRECISION);
	long int precision_base = precision;
	if(base != 10 && base >= 2 && base <= 36) {
		Number precmax(10);
		precmax.raise(precision_base);
		precmax--;
		precmax.log(base);
		precmax.floor();
		precision_base = precmax.lintValue();
	}
	long int i_precision_base = precision_base;
	if((i_precision < 0 && FROM_BIT_PRECISION(BIT_PRECISION) > precision) || i_precision > precision) {
		if(i_precision < 0) i_precision_base = FROM_BIT_PRECISION(BIT_PRECISION);
		else i_precision_base = i_precision;
		if(base != 10 && base >= 2 && base <= 36) {
			Number precmax(10);
			precmax.raise(i_precision_base);
			precmax--;
			precmax.log(base);
			precmax.floor();
			i_precision_base = precmax.lintValue();
		}
	}
	if(po.restrict_to_parent_precision && ips.parent_precision > 0 && ips.parent_precision < precision) precision = ips.parent_precision;
	bool approx = isApproximate() || (ips.parent_approximate && po.restrict_to_parent_precision);
	
	if(isInteger()) {
		
		long int length = mpz_sizeinbase(mpq_numref(r_value), base);
		if(precision_base + min_decimals + 1000 + ::abs(po.min_exp) < length && ((approx || (po.min_exp != 0 && (po.restrict_fraction_length || po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT))) || length > 1000000L)) {
			Number nr(*this);
			if(nr.setToFloatingPoint()) {
				str = nr.print(po, ips);
				return str;
			}
		}
		
		mpz_t ivalue;
		mpz_init_set(ivalue, mpq_numref(r_value));
		bool neg = (mpz_sgn(ivalue) < 0);
		bool rerun = false;
		bool exact = true;

		integer_rerun:
		
		string mpz_str = printMPZ(ivalue, base, false, BASE_DISPLAY_NONE, po.lower_case_numbers, po.use_unicode_signs);
		if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
		
		length = mpz_str.length();
		
		long int expo = 0;
		if(base == 10) {
			if(mpz_str.length() > 0 && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
				expo = length - 1;
			} else if(mpz_str.length() > 0) {
				for(long int i = mpz_str.length() - 1; i >= 0; i--) {
					if(mpz_str[i] != '0') {
						break;
					}
					expo++;
				} 
			}
			if(po.min_exp == EXP_PRECISION) {	
				long int precexp = i_precision_base;
				if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
				else if(precexp > precision + 3) precexp = precision + 3;
				if((expo > 0 && expo < precexp) || (expo < 0 && expo > -PRECISION)) {
					expo = 0;
				}
			} else if(po.min_exp < -1) {
				expo -= expo % (-po.min_exp);
				if(expo < 0) expo = 0;
			} else if(po.min_exp != 0) {
				if((long int) expo > -po.min_exp && (long int) expo < po.min_exp) { 
					expo = 0;
				}
			} else {
				expo = 0;
			}
		}
		long int decimals = expo;
		long int nondecimals = length - decimals;

		bool dp_added = false;

		if(!rerun && mpz_sgn(ivalue) != 0) {
			long int precision2 = precision_base;
			if(min_decimals > 0 && min_decimals + nondecimals > precision_base) {
				precision2 = min_decimals + nondecimals;
				if(approx && precision2 > i_precision_base) precision2 = i_precision_base;
			}
			if(po.use_max_decimals && po.max_decimals >= 0 && decimals > po.max_decimals && (!approx || po.max_decimals + nondecimals < precision2) && (po.restrict_fraction_length || (base == 10 && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)))) {
				mpz_t i_rem, i_quo, i_div;
				mpz_inits(i_rem, i_quo, i_div, NULL);
				mpz_ui_pow_ui(i_div, (unsigned long int) base, (unsigned long int) -(po.max_decimals - expo));
				mpz_fdiv_qr(i_quo, i_rem, ivalue, i_div);
				if(mpz_sgn(i_rem) != 0) {
					mpz_set(ivalue, i_quo);
					mpq_t q_rem, q_base_half;
					mpq_inits(q_rem, q_base_half, NULL);
					mpz_set(mpq_numref(q_rem), i_rem);
					mpz_set(mpq_denref(q_rem), i_div);
					mpz_set_si(mpq_numref(q_base_half), base);
					mpq_mul(q_rem, q_rem, q_base_half);
					mpz_set_ui(mpq_denref(q_base_half), 2);
					int i_sign = mpq_cmp(q_rem, q_base_half);
					if(po.round_halfway_to_even && mpz_even_p(ivalue)) {
						if(i_sign > 0) mpz_add_ui(ivalue, ivalue, 1);
					} else {
						if(i_sign >= 0) mpz_add_ui(ivalue, ivalue, 1);
					}
					mpq_clears(q_base_half, q_rem, NULL);
					mpz_mul(ivalue, ivalue, i_div);
					exact = false;
					rerun = true;
					mpz_clears(i_rem, i_quo, i_div, NULL);
					goto integer_rerun;
				}
				mpz_clears(i_rem, i_quo, i_div, NULL);
			} else if(precision2 < length && (approx || po.restrict_fraction_length || (base == 10 && expo != 0 && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)))) {

				mpq_t qvalue;
				mpq_init(qvalue);
				mpz_set(mpq_numref(qvalue), ivalue);
				
				precision2 = length - precision2;

				long int p2_cd = precision2;
				
				mpq_t q_exp;
				mpq_init(q_exp);

				long int p2_cd_min = 10000;
				while(p2_cd_min >= 1000) {
					if(p2_cd > p2_cd_min) {
						mpz_ui_pow_ui(mpq_numref(q_exp), (unsigned long int) base, (unsigned long int) p2_cd_min);
						while(p2_cd > p2_cd_min) {
							mpq_div(qvalue, qvalue, q_exp);
							p2_cd -= p2_cd_min;
							if(CALCULATOR->aborted()) {mpq_clears(q_exp, qvalue, NULL); mpz_clear(ivalue); return CALCULATOR->abortedMessage();}
						}
					}
					p2_cd_min = p2_cd_min / 10;
				}
				
				mpz_ui_pow_ui(mpq_numref(q_exp), (unsigned long int) base, (unsigned long int) p2_cd);
				mpq_div(qvalue, qvalue, q_exp);
					
				mpz_t i_rem, i_quo;
				mpz_inits(i_rem, i_quo, NULL);
				mpz_fdiv_qr(i_quo, i_rem, mpq_numref(qvalue), mpq_denref(qvalue));
				if(mpz_sgn(i_rem) != 0) {
					mpz_set(ivalue, i_quo);
					mpq_t q_rem, q_base_half;
					mpq_inits(q_rem, q_base_half, NULL);
					mpz_set(mpq_numref(q_rem), i_rem);
					mpz_set(mpq_denref(q_rem), mpq_denref(qvalue));
					mpz_set_si(mpq_numref(q_base_half), base);
					mpq_mul(q_rem, q_rem, q_base_half);
					mpz_set_ui(mpq_denref(q_base_half), 2);
					int i_sign = mpq_cmp(q_rem, q_base_half);
					if(po.round_halfway_to_even && mpz_even_p(ivalue)) {
						if(i_sign > 0) mpz_add_ui(ivalue, ivalue, 1);
					} else {
						if(i_sign >= 0) mpz_add_ui(ivalue, ivalue, 1);
					}
					mpq_clears(q_base_half, q_rem, NULL);
					mpz_ui_pow_ui(i_quo, (unsigned long int) base, (unsigned long int) precision2);
					mpz_mul(ivalue, ivalue, i_quo);
					exact = false;
					rerun = true;
					mpz_clears(i_rem, i_quo, NULL);
					mpq_clears(q_exp, qvalue, NULL);
					goto integer_rerun;
				}
				mpz_clears(i_rem, i_quo, NULL);
				mpq_clears(q_exp, qvalue, NULL);
			}
		}

		mpz_clear(ivalue);

		decimals = 0;
		if(expo > 0) {
			if(po.number_fraction_format == FRACTION_DECIMAL) {
				mpz_str.insert(mpz_str.length() - expo, po.decimalpoint());
				dp_added = true;
				decimals = expo;
			} else if(po.number_fraction_format == FRACTION_DECIMAL_EXACT) {
				mpz_str.insert(mpz_str.length() - expo, po.decimalpoint());
				dp_added = true;
				decimals = expo;
			} else {
				mpz_str = mpz_str.substr(0, mpz_str.length() - expo);
			}
		}
		if(ips.minus) *ips.minus = neg;
		str = format_number_string(mpz_str, base, po.base_display, !ips.minus && neg);
		if(base != BASE_ROMAN_NUMERALS && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
			int pos = str.length() - 1;
			for(; pos >= (int) str.length() + min_decimals - decimals; pos--) {
				if(str[pos] != '0') {
					break;
				}
			}
			if(pos + 1 < (int) str.length()) {
				decimals -= str.length() - (pos + 1);
				str = str.substr(0, pos + 1);
			}
			if(exact && min_decimals > decimals) {
				if(decimals <= 0) {
					str += po.decimalpoint();
					dp_added = true;
				}
				while(min_decimals > decimals) {
					decimals++;
					str += "0";
				}
			}
			if(str[str.length() - 1] == po.decimalpoint()[0]) {
				str.erase(str.end() - 1);
				dp_added = false;
			}
		}

		if(!exact && po.is_approximate) *po.is_approximate = true;
		if(po.show_ending_zeroes && (!exact || approx) && (!po.use_max_decimals || po.max_decimals < 0 || po.max_decimals > decimals)) {
			precision = precision_base;
			precision -= str.length();
			if(dp_added) {
				precision += 1;
			} else if(precision > 0) {
				str += po.decimalpoint();
			}
			for(; precision > 0 && (!po.use_max_decimals || po.max_decimals < 0 || po.max_decimals > decimals); precision--) {
				decimals++;
				str += "0";
			}
		}
		if(expo != 0) {
			if(ips.iexp) *ips.iexp = expo;
			if(ips.exp) {
				if(ips.exp_minus) {
					*ips.exp_minus = expo < 0;
					if(expo < 0) expo = -expo;
				}
				*ips.exp = i2s(expo);
			} else {
				if(po.lower_case_e) str += "e";
				else str += "E";
				str += i2s(expo);
			}
		}
		if(ips.num) *ips.num = str;

	} else if(isInfinity()) {
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_INFINITY, po.can_display_unicode_string_arg))) {
			str = SIGN_INFINITY;
		} else {
			str = _("infinity");
		}
	} else if(isPlusInfinity()) {
		str += "+";
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_INFINITY, po.can_display_unicode_string_arg))) {
			str += SIGN_INFINITY;
		} else {
			str += _("infinity");
		}
	} else if(isMinusInfinity()) {
		str += "-";
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_INFINITY, po.can_display_unicode_string_arg))) {
			str += SIGN_INFINITY;
		} else {
			str += _("infinity");
		}
	} else if(n_type == NUMBER_TYPE_FLOAT) {

		bool rerun = false;
		if(base < 2 || base > 36) base = 10;
		mpfr_clear_flags();
		
		if(po.interval_display == INTERVAL_DISPLAY_SIGNIFICANT_DIGITS) {
			PrintOptions po3 = po;
			po3.interval_display = INTERVAL_DISPLAY_INTERVAL;
			cout << print(po3) << endl;
		}
		
		mpfr_t f_diff, f_mid;
		
		if(!mpfr_equal_p(fl_value, fu_value)) cout << "INTERVAL" << endl;
		
		if(mpfr_equal_p(fl_value, fu_value)) {
			mpfr_init2(f_mid, mpfr_get_prec(fl_value));
			mpfr_set(f_mid, fl_value, MPFR_RNDN);
		} else if(po.interval_display == INTERVAL_DISPLAY_INTERVAL) {
			PrintOptions po2 = po;
			po2.interval_display = INTERVAL_DISPLAY_LOWER;
			string str1 = print(po2);
			po2.interval_display = INTERVAL_DISPLAY_UPPER;
			string str2 = print(po2);
			if(str1 == str2) return print(po2, ips);
			str = CALCULATOR->f_interval->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name;
			str += LEFT_PARENTHESIS;
			str += str1;
			str += COMMA SPACE;
			str += str2;
			str += RIGHT_PARENTHESIS;
			if(ips.minus) *ips.minus = false;
			if(ips.num) *ips.num = str;
			return str;
		} else if(po.interval_display == INTERVAL_DISPLAY_PLUSMINUS) {
			mpfr_t f_mid, f_diff, f_log, f_log_diff;
			mpfr_inits2(BIT_PRECISION, f_mid, f_diff, f_log, f_log_diff, NULL);
			mpfr_sub(f_diff, fu_value, fl_value, MPFR_RNDU);
			mpfr_div_ui(f_diff, f_diff, 2, MPFR_RNDU);
			mpfr_add(f_mid, fl_value, f_diff, MPFR_RNDN);
			mpfr_log10(f_log, f_mid, MPFR_RNDN);
			mpfr_floor(f_log, f_log);
			mpfr_log10(f_log_diff, f_diff, MPFR_RNDN);
			mpfr_floor(f_log_diff, f_log_diff);
			mpfr_sub(f_log_diff, f_log_diff, f_log, MPFR_RNDN);
			long int i_log_diff = mpfr_get_si(f_log_diff, MPFR_RNDN);

			PrintOptions po2 = po;
			if(precision + i_log_diff > 0) {
				long int iexp = 0, deci = 0;
				InternalPrintStruct ips2 = ips;
				ips2.iexp = &iexp;
				ips2.decimals = &deci;
				po2.interval_display = INTERVAL_DISPLAY_MIDPOINT;
				po2.min_decimals = 0;
				po2.use_max_decimals = false;
				str = print(po2, ips2);
				if(ips.iexp) *ips.iexp = iexp;
				if(ips.decimals) *ips.decimals = deci;
				str += "±";
				po2.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
				precision += i_log_diff;
				if(iexp > 0) {
					mpfr_ui_pow_ui(f_log, 10, iexp, MPFR_RNDN);
					mpfr_div(f_diff, f_diff, f_log, MPFR_RNDN);
				} else if(iexp < 0) {
					mpfr_ui_pow_ui(f_log, 10, -iexp, MPFR_RNDN);
					mpfr_mul(f_diff, f_diff, f_log, MPFR_RNDN);
				} else {
					long int i_log = mpfr_get_si(f_log, MPFR_RNDN);
					if(i_log < 0) i_log = -i_log;
					if(i_log + 1 > precision) precision = i_log + i_log_diff + 1;
				}
				po2.min_exp = 0;
				Number nr;
				nr.setInternal(f_diff);
				if(po.preserve_precision) {
					if(i_precision < 0) precision = FROM_BIT_PRECISION(BIT_PRECISION);
					precision += i_log_diff;
					if(precision < 0) precision = 0;
				}
				nr.setPrecision(precision);
				str += nr.print(po2);
				if(iexp != 0 && !ips.exp) {
					if(po.lower_case_e) str += "e";
					else str += "E";
					str += i2s(iexp);
				}
			} else {
				po2.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
				str = print(po2, ips);
			}
			if(ips.num) *ips.num = str;
			mpfr_clears(f_diff, f_mid, f_log, f_log_diff, NULL);
			return str;
		} else if(po.interval_display == INTERVAL_DISPLAY_MIDPOINT) {
			mpfr_inits2(mpfr_get_prec(fl_value), f_diff, f_mid, NULL);
			mpfr_sub(f_diff, fu_value, fl_value, MPFR_RNDN);
			mpfr_div_ui(f_diff, f_diff, 2, MPFR_RNDN);
			mpfr_add(f_mid, fl_value, f_diff, MPFR_RNDN);
			mpfr_clear(f_diff);
			if(po.is_approximate) *po.is_approximate = true;
		} else if(po.interval_display == INTERVAL_DISPLAY_LOWER) {
			mpfr_init2(f_mid, mpfr_get_prec(fl_value));
			mpfr_set(f_mid, fl_value, MPFR_RNDD);
		} else if(po.interval_display == INTERVAL_DISPLAY_UPPER) {
			mpfr_init2(f_mid, mpfr_get_prec(fu_value));
			mpfr_set(f_mid, fu_value, MPFR_RNDU);
		} else {
			mpfr_t vl, vu, f_logl, f_base, f_log_base;
			mpfr_inits2(mpfr_get_prec(fl_value), f_mid, vl, vu, f_logl, f_base, f_log_base, NULL);
		
			mpfr_set_si(f_base, base, MPFR_RNDN);
			mpfr_log(f_log_base, f_base, MPFR_RNDN);
			
			mpq_t base_half;
			mpq_init(base_half);
			mpq_set_ui(base_half, base, 2);
			mpq_canonicalize(base_half);
		
			if(mpfr_sgn(fl_value) != mpfr_sgn(fu_value)) {
				long int ilogl = i_precision_base, ilogu = i_precision_base;
				if(mpfr_sgn(fl_value) < 0) {
					mpfr_neg(f_logl, fl_value, MPFR_RNDU);
					mpfr_log(f_logl, f_logl, MPFR_RNDU);
					mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDU);
					ilogl = -mpfr_get_si(f_logl, MPFR_RNDU);
					if(ilogl >= 0) {
						mpfr_ui_pow_ui(f_logl, (unsigned long int) base, (unsigned long int) ilogl + 1, MPFR_RNDU);
						mpfr_mul(vl, fl_value, f_logl, MPFR_RNDD);
						mpfr_neg(vl, vl, MPFR_RNDU);
						if(mpfr_cmp_q(vl, base_half) <= 0) ilogl++;
					}
				}
				if(mpfr_sgn(fu_value) > 0) {
					mpfr_log(f_logl, fu_value, MPFR_RNDU);
					mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDU);
					ilogu = -mpfr_get_si(f_logl, MPFR_RNDU);
					if(ilogu >= 0) {
						mpfr_ui_pow_ui(f_logl, (unsigned long int) base, (unsigned long int) ilogu + 1, MPFR_RNDU);
						mpfr_mul(vu, fu_value, f_logl, MPFR_RNDU);
						if(mpfr_cmp_q(vu, base_half) <= 0) ilogu++;
					}
				}
				mpfr_clears(vu, vl, f_logl, f_mid, f_base, f_log_base, NULL);
				mpq_clear(base_half);
				if(ilogu < ilogl) ilogl = ilogu;
				if(ilogl <= 0) {
					PrintOptions po2 = po;
					po2.interval_display = INTERVAL_DISPLAY_INTERVAL;
					return print(po2, ips);
				} else {
					i_precision_base = ilogl;
				}
				Number nr_zero;
				nr_zero.setApproximate(true);
				PrintOptions po2 = po;
				po2.max_decimals = i_precision_base - 1;
				po2.use_max_decimals = true;
				return nr_zero.print(po2, ips);
			}
			
			float_interval_prec_rerun:
			
			mpfr_set(vl, fl_value, MPFR_RNDN);
			mpfr_set(vu, fu_value, MPFR_RNDN);
			bool negl = (mpfr_sgn(vl) < 0);
			if(negl) {
				mpfr_neg(vl, vl, MPFR_RNDN);
				mpfr_neg(vu, vu, MPFR_RNDN);
				mpfr_swap(vl, vu);
			}
			mpfr_nextbelow(vu);
			mpfr_log(f_logl, vu, MPFR_RNDN);
			mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDN);
			mpfr_floor(f_logl, f_logl);
			mpfr_sub_si(f_logl, f_logl, i_precision_base, MPFR_RNDN);
			mpfr_pow(f_logl, f_base, f_logl, MPFR_RNDN);
			mpfr_div(vl, vl, f_logl, MPFR_RNDU);
			mpfr_div(vu, vu, f_logl, MPFR_RNDD);
			if(mpfr_cmp(vl, vu) > 0) mpfr_swap(vl, vu);
			mpfr_round(vl, vl);
			mpfr_round(vu, vu);
			mpz_t ivalue;
			mpz_init(ivalue);
			mpfr_get_z(ivalue, vl, MPFR_RNDN);
			string str_l = printMPZ(ivalue, base, true, BASE_DISPLAY_NONE, false, false);
			mpfr_get_z(ivalue, vu, MPFR_RNDN);
			string str_u = printMPZ(ivalue, base, true, BASE_DISPLAY_NONE, false, false);
			cout << str_l << endl;
			cout << str_u << endl;
			
			PRINT_MPFR(fl_value, 2);
			PRINT_MPFR(fu_value, 2);
			
			if(str_u.length() > str_l.length()) {
				str_l.insert(0, str_u.length() - str_l.length(), '0');
			}
			for(size_t i = 0; i < str_l.size(); i++) {
				if(str_l[i] != str_u[i]) {
					if(char2val(str_l[i], base) + 1 == char2val(str_u[i], base)) {
						i++;
						bool do_rerun = false;
						for(; i < str_l.size(); i++) {
							if(char2val(str_l[i], base) == base - 1) {
								do_rerun = true;
							} else if(char2val(str_l[i], base) >= base / 2) {
								do_rerun = true;
								break;
							} else {
								i--;
								break;
							}
						}
						if(do_rerun) {
							if(i_precision_base == 0 || i == 0) {i_precision_base = 0; break;}
							else if(i > (size_t) i_precision_base) i_precision_base--;
							else i_precision_base = i - 1;
							goto float_interval_prec_rerun;
						}
					}
					if(i == 0 || i_precision_base <= 0) {
						if(i_precision_base < 0) break;
						mpfr_mul(f_mid, vu, f_logl, MPFR_RNDN);
						if(mpfr_cmp_ui(f_mid, 1) >= 0) {i_precision_base = 0; break;}
						i_precision_base = -1;
					} else if(i - 1 >= (size_t) i_precision_base) i_precision_base--;
					else i_precision_base = i - 1;
					goto float_interval_prec_rerun;
				} else if(i == str_l.size() - 1) {
					i_precision_base = i + 1;
				}
			}

			if(i_precision_base < precision_base) precision_base = i_precision_base;
			
			if(i_precision_base <= 0) {
				if(negl) {
					mpfr_neg(vl, fl_value, MPFR_RNDN);
					mpfr_neg(vu, fu_value, MPFR_RNDN);
				} else {
					mpfr_set(vl, fl_value, MPFR_RNDN);
					mpfr_set(vu, fu_value, MPFR_RNDN);
				}
				mpfr_log(f_logl, vl, MPFR_RNDU);
				mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDU);
				long int ilogl = -mpfr_get_si(f_logl, MPFR_RNDU);
				if(ilogl >= 0) {
					mpfr_ui_pow_ui(f_logl, (unsigned long int) base, (unsigned long int) ilogl + 1, MPFR_RNDU);
					mpfr_mul(vl, fl_value, f_logl, MPFR_RNDD);
					mpfr_neg(vl, vl, MPFR_RNDU);
					if(mpfr_cmp_q(vl, base_half) <= 0) ilogl++;
				}
				mpfr_log(f_logl, vu, MPFR_RNDU);
				mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDU);
				long int ilogu = -mpfr_get_si(f_logl, MPFR_RNDU);
				if(ilogu >= 0) {
					mpfr_ui_pow_ui(f_logl, (unsigned long int) base, (unsigned long int) ilogu + 1, MPFR_RNDU);
					mpfr_mul(vu, fu_value, f_logl, MPFR_RNDU);
					if(mpfr_cmp_q(vu, base_half) <= 0) ilogu++;
				}
				mpfr_clears(vu, vl, f_logl, f_mid, f_base, f_log_base, NULL);
				mpq_clear(base_half);
				if(ilogu < ilogl) ilogl = ilogu;
				if(ilogl <= 0) {
					PrintOptions po2 = po;
					po2.interval_display = INTERVAL_DISPLAY_INTERVAL;
					return print(po2, ips);
				} else {
					i_precision_base = ilogl;
				}
				Number nr_zero;
				nr_zero.setApproximate(true);
				PrintOptions po2 = po;
				po2.max_decimals = i_precision_base - 1;
				po2.use_max_decimals = true;
				return nr_zero.print(po2, ips);
			}
			
			mpfr_mul(f_mid, vu, f_logl, MPFR_RNDN);
			if(negl) mpfr_neg(f_mid, f_mid, MPFR_RNDN);
			
			mpfr_clears(vl, vu, f_logl, f_base, f_log_base, NULL);
			mpq_clear(base_half);
				
			if(po.is_approximate) *po.is_approximate = true;
		}
		
		precision = precision_base;
		
		cout << i_precision_base << ":" << precision << endl;
		
		if(mpfr_zero_p(f_mid)) {
			Number nr_zero;
			nr_zero.setApproximate(true);
			PrintOptions po2 = po;
			po2.max_decimals = i_precision_base - 1;
			po2.use_max_decimals = true;
			return nr_zero.print(po2, ips);
		}

		float_rerun:
		
		mpfr_t v;
		mpfr_init2(v, mpfr_get_prec(f_mid));
		mpfr_t f_log, f_base, f_log_base;
		mpfr_inits2(mpfr_get_prec(f_mid), f_log, f_base, f_log_base, NULL);
		mpfr_set_si(f_base, base, MPFR_RNDN);
		mpfr_log(f_log_base, f_base, MPFR_RNDN);
		long int expo = 0;
		long int l10 = 0;
		mpfr_set(v, f_mid, MPFR_RNDN);
		bool neg = (mpfr_sgn(v) < 0);
		if(neg) mpfr_neg(v, v, MPFR_RNDN);
		mpfr_log(f_log, v, MPFR_RNDN);
		mpfr_div(f_log, f_log, f_log_base, MPFR_RNDN);
		mpfr_floor(f_log, f_log);
		long int i_log = mpfr_get_si(f_log, MPFR_RNDN);
		if(base == 10) {
			expo = i_log;
			if(po.min_exp == EXP_PRECISION || (po.min_exp == 0 && expo > 1000000L)) {
				long int precexp = i_precision_base;
				if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
				else if(precexp > precision + 3) precexp = precision + 3;
				if((expo > 0 && expo < precexp) || (expo < 0 && expo > -PRECISION)) {
					expo = 0;
				}
			} else if(po.min_exp < -1) {
				if(expo < 0) {
					int expo_rem = (-expo) % (-po.min_exp);
					if(expo_rem > 0) expo_rem = (-po.min_exp) - expo_rem;
					expo -= expo_rem;
					if(expo > 0) expo = 0;
				} else if(expo > 0) {
					expo -= expo % (-po.min_exp);
					if(expo < 0) expo = 0;
				}
			} else if(po.min_exp != 0) {
				if(expo > -po.min_exp && expo < po.min_exp) { 
					expo = 0;
				}
			} else {
				expo = 0;
			}
		}
		if(!rerun && i_precision_base > precision_base && min_decimals > 0) {
			if(min_decimals > precision - 1 - (i_log - expo)) {
				precision = min_decimals + 1 + (i_log - expo);
				if(precision > i_precision_base) precision = i_precision_base;
				mpfr_clears(v, f_log, f_base, f_log_base, NULL);
				rerun = true;
				goto float_rerun;
			}
		}
		if(expo == 0 && i_log > precision) {
			precision = (i_precision_base > i_log + 1) ? i_log + 1 : i_precision_base;
		}
		mpfr_sub_si(f_log, f_log, (po.use_max_decimals && po.max_decimals >= 0 && precision > po.max_decimals + i_log - expo) ? po.max_decimals + i_log - expo: precision - 1, MPFR_RNDN);
		l10 = expo - mpfr_get_si(f_log, MPFR_RNDN);
		mpfr_pow(f_log, f_base, f_log, MPFR_RNDN);
		mpfr_div(v, v, f_log, MPFR_RNDN);
		if(po.round_halfway_to_even) mpfr_rint(v, v, MPFR_RNDN);
		else mpfr_round(v, v);
		mpz_t ivalue;
		mpz_init(ivalue);
		mpfr_get_z(ivalue, v, MPFR_RNDN);

		str = printMPZ(ivalue, base, true, BASE_DISPLAY_NONE, po.lower_case_numbers, po.use_unicode_signs);
		
		bool has_decimal = false;
		if(l10 > 0) {
			l10 = str.length() - l10;
			if(l10 < 1) {
				str.insert(str.begin(), 1 - l10, '0');
				l10 = 1;
			}
			str.insert(l10, po.decimalpoint());
			has_decimal = true;
			int l2 = 0;
			while(str[str.length() - 1 - l2] == '0') {
				l2++;
			}
			if(l2 > 0 && !po.show_ending_zeroes) {
				if(min_decimals > 0) {
					int decimals = str.length() - l10 - 1;
					if(decimals - min_decimals < l2) l2 = decimals - min_decimals;
				}
				if(l2 > 0) str = str.substr(0, str.length() - l2);
			}
			if(str[str.length() - 1] == po.decimalpoint()[0]) {
				str.erase(str.end() - 1);
				has_decimal = false;
			}
		} else if(l10 < 0) {
			while(l10 < 0) {
				l10++;
				str += "0";
			}
		}

		if(str.empty()) {
			str = "0";
		}
		if(str[str.length() - 1] == po.decimalpoint()[0]) {
			str.erase(str.end() - 1);
			has_decimal = false;
		}
		
		str = format_number_string(str, base, po.base_display, !ips.minus && neg, !has_decimal);
		
		if(expo != 0) {
			if(ips.iexp) *ips.iexp = expo;
			if(ips.exp) {
				if(ips.exp_minus) {
					*ips.exp_minus = expo < 0;
					if(expo < 0) expo = -expo;
				}
				*ips.exp = i2s(expo);
			} else {
				if(po.lower_case_e) str += "e";
				else str += "E";
				str += i2s(expo);
			}
		}
		if(ips.minus) *ips.minus = neg;
		if(ips.num) *ips.num = str;
		mpfr_clears(f_mid, v, f_log, f_base, f_log_base, NULL);
		if(po.is_approximate && mpfr_inexflag_p()) *po.is_approximate = true;
		testErrors(2);

	} else if(base != BASE_ROMAN_NUMERALS && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
		long int numlength = mpz_sizeinbase(mpq_numref(r_value), base);
		long int denlength = mpz_sizeinbase(mpq_denref(r_value), base);
		if(precision_base + min_decimals + 1000 + ::abs(po.min_exp) < labs(numlength - denlength) && (approx || po.min_exp != 0)) {
			Number nr(*this);
			if(nr.setToFloatingPoint()) {
				str = nr.print(po, ips);
				return str;
			}
		}
		mpz_t num, d, remainder, remainder2, exp;
		mpz_inits(num, d, remainder, remainder2, exp, NULL);
		mpz_set(d, mpq_denref(r_value));
		mpz_set(num, mpq_numref(r_value));
		bool neg = (mpz_sgn(num) < 0);
		if(neg) mpz_neg(num, num);
		mpz_tdiv_qr(num, remainder, num, d);
		bool exact = (mpz_sgn(remainder) == 0);
		vector<mpz_t*> remainders;
		bool started = false;
		long int expo = 0;
		long int precision2 = precision;
		int num_sign = mpz_sgn(num);
		if(num_sign != 0) {

			str = printMPZ(num, base, true, BASE_DISPLAY_NONE);

			if(CALCULATOR->aborted()) {mpz_clears(num, d, remainder, remainder2, exp, NULL); return CALCULATOR->abortedMessage();}
			
			long int length = str.length();
			if(base != 10) {
				expo = 0;
			} else {
				expo = length - 1;
				if(po.min_exp == EXP_PRECISION) {
					long int precexp = i_precision_base;
					if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
					else if(precexp > precision + 3) precexp = precision + 3;
					if((expo > 0 && expo < precexp) || (expo < 0 && expo > -PRECISION)) {
						expo = 0;
					}
				} else if(po.min_exp < -1) {
					expo -= expo % (-po.min_exp);
					if(expo < 0) expo = 0;
				} else if(po.min_exp != 0) {
					if(expo > -po.min_exp && expo < po.min_exp) { 
						expo = 0;
					}
				} else {
					expo = 0;
				}
			}
			long int decimals = expo;
			long int nondecimals = length - decimals;
			
			precision2 -= length;
			if(!approx && min_decimals + nondecimals > precision) precision2 = (min_decimals + nondecimals) - length;
			
			int do_div = 0;
			
			if(po.use_max_decimals && po.max_decimals >= 0 && decimals > po.max_decimals && (!approx || po.max_decimals - decimals < precision2)) {
				do_div = 1;
			} else if(precision2 < 0 && (approx || decimals > min_decimals)) {
				do_div = 2;
			}
			if(do_div) {
				mpz_t i_rem, i_quo, i_div, i_div_pre;
				mpz_inits(i_rem, i_quo, i_div, i_div_pre, NULL);
				mpz_ui_pow_ui(i_div_pre, (unsigned long int) base, do_div == 1 ? (unsigned long int) -(po.max_decimals - decimals) : (unsigned long int) -precision2);
				mpz_mul(i_div, i_div_pre, mpq_denref(r_value));
				mpz_fdiv_qr(i_quo, i_rem, mpq_numref(r_value), i_div);
				if(mpz_sgn(i_rem) != 0) {
					mpz_set(num, i_quo);
					mpq_t q_rem, q_base_half;
					mpq_inits(q_rem, q_base_half, NULL);
					mpz_set(mpq_numref(q_rem), i_rem);
					mpz_set(mpq_denref(q_rem), i_div);
					mpz_set_si(mpq_numref(q_base_half), base);
					mpq_mul(q_rem, q_rem, q_base_half);
					mpz_set_ui(mpq_denref(q_base_half), 2);
					int i_sign = mpq_cmp(q_rem, q_base_half);
					if(po.round_halfway_to_even && mpz_even_p(num)) {
						if(i_sign > 0) mpz_add_ui(num, num, 1);
					} else {
						if(i_sign >= 0) mpz_add_ui(num, num, 1);
					}
					mpq_clears(q_base_half, q_rem, NULL);
					mpz_mul(num, num, i_div_pre);
					exact = false;
					if(neg) mpz_neg(num, num);
				}
				mpz_clears(i_rem, i_quo, i_div, i_div_pre, NULL);
				mpz_set_ui(remainder, 0);
			}
			started = true;
			if(po.use_max_decimals && po.max_decimals >= 0 && precision2 > po.max_decimals - decimals) precision2 = po.max_decimals - decimals;
		}

		bool try_infinite_series = po.indicate_infinite_series && !isFloatingPoint();

		mpz_t remainder_bak, num_bak;
		if(num_sign == 0 && ((po.use_max_decimals && po.max_decimals > 0) || min_decimals > 0)) {
			mpz_init_set(remainder_bak, remainder);
			mpz_init_set(num_bak, num);
		}
		bool rerun = false;
		
		rational_rerun:

		bool infinite_series = false;
		long int l10 = 0;
		if(rerun) {
			mpz_set(num, num_bak);
			mpz_set(remainder, remainder_bak);
		}
		while(!exact && precision2 > 0) {
			if(try_infinite_series) {
				mpz_t *remcopy = new mpz_t[1];
				mpz_init_set(*remcopy, remainder);
				remainders.push_back(remcopy);
			}
			mpz_mul_si(remainder, remainder, base);
			mpz_tdiv_qr(remainder, remainder2, remainder, d);
			exact = (mpz_sgn(remainder2) == 0);
			if(!started) {
				started = (mpz_sgn(remainder) != 0);
			}
			if(started) {
				mpz_mul_si(num, num, base);
				mpz_add(num, num, remainder);
			}
			if(CALCULATOR->aborted()) {
				if(num_sign == 0 && po.use_max_decimals && po.max_decimals > 0) mpz_clears(num_bak, remainder_bak, NULL); 
				mpz_clears(num, d, remainder, remainder2, exp, NULL); 
				return CALCULATOR->abortedMessage();
			}
			l10++;
			mpz_set(remainder, remainder2);
			if(try_infinite_series && !exact) {
				for(size_t i = 0; i < remainders.size(); i++) {
					if(CALCULATOR->aborted()) {mpz_clears(num, d, remainder, remainder2, exp, NULL); return CALCULATOR->abortedMessage();}
					if(!mpz_cmp(*remainders[i], remainder)) {
						infinite_series = true;
						try_infinite_series = false;
						break;
					}
				}
			}
			if(started) {
				precision2--;
			}
		}
		for(size_t i = 0; i < remainders.size(); i++) {
			mpz_clear(*remainders[i]);
			free(remainders[i]);
		}
		remainders.clear();
		if(!exact && !infinite_series) {
			mpz_mul_si(remainder, remainder, base);
			mpz_tdiv_qr(remainder, remainder2, remainder, d);
			mpq_t q_rem, q_base_half;
			mpq_inits(q_rem, q_base_half, NULL);
			mpz_set(mpq_numref(q_rem), remainder);
			mpz_set_si(mpq_numref(q_base_half), base);
			mpz_set_ui(mpq_denref(q_base_half), 2);
			int i_sign = mpq_cmp(q_rem, q_base_half);
			if(po.round_halfway_to_even && mpz_sgn(remainder2) == 0 && mpz_even_p(num)) {
				if(i_sign > 0) mpz_add_ui(num, num, 1);
			} else {
				if(i_sign >= 0) mpz_add_ui(num, num, 1);
			}
			mpq_clears(q_base_half, q_rem, NULL);
		}
		if(!exact && !infinite_series) {
			if(po.number_fraction_format == FRACTION_DECIMAL_EXACT && !isApproximate()) {
				PrintOptions po2 = po;
				po2.number_fraction_format = FRACTION_FRACTIONAL;
				if(expo != 0) po2.restrict_fraction_length = true;
				if(num_sign == 0 && po.use_max_decimals && po.max_decimals > 0) mpz_clears(num_bak, remainder_bak, NULL); 
				mpz_clears(num, d, remainder, remainder2, exp, NULL);
				return print(po2, ips);
			}
			if(po.is_approximate) *po.is_approximate = true;
		}
		str = printMPZ(num, base, true, BASE_DISPLAY_NONE, po.lower_case_numbers, po.use_unicode_signs);
		if(base == 10 && !rerun) {
			expo = str.length() - l10 - 1;
			if(po.min_exp == EXP_PRECISION) {
				long int precexp = i_precision_base;
				if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
				else if(precexp > precision + 3) precexp = precision + 3;
				if((expo > 0 && expo < precexp) || (expo < 0 && expo > -PRECISION)) {
					expo = 0;
				}
			} else if(po.min_exp < -1) {
				if(expo < 0) {
					int expo_rem = (-expo) % (-po.min_exp);
					if(expo_rem > 0) expo_rem = (-po.min_exp) - expo_rem;
					expo -= expo_rem;
					if(expo > 0) expo = 0;
				} else if(expo > 0) {
					expo -= expo % (-po.min_exp);
					if(expo < 0) expo = 0;
				}
			} else if(po.min_exp != 0) {
				if(expo > -po.min_exp && expo < po.min_exp) { 
					expo = 0;
				}
			} else {
				expo = 0;
			}
		}
		if(!rerun && num_sign == 0 && expo <= 0 && po.use_max_decimals && po.max_decimals >= 0 && l10 + expo > po.max_decimals) {
			precision2 = po.max_decimals + (str.length() - l10 - expo);
			rerun = true;
			exact = false;
			started = false;
			goto rational_rerun;
		}
		if(!rerun && !approx && !exact && num_sign == 0 && expo < 0 && min_decimals > 0 && l10 + expo < min_decimals) {
			precision2 = min_decimals + (str.length() - l10 - expo);
			rerun = true;
			started = false;
			goto rational_rerun;
		}
		if(expo != 0) {
			l10 += expo;
		}
		if(num_sign == 0 && po.use_max_decimals && po.max_decimals > 0) mpz_clears(num_bak, remainder_bak, NULL); 
		mpz_clears(num, d, remainder, remainder2, exp, NULL);
		if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
		bool has_decimal = false;
		if(l10 > 0) {
			l10 = str.length() - l10;
			if(l10 < 1) {
				str.insert(str.begin(), 1 - l10, '0');
				l10 = 1;
			}				
			str.insert(l10, po.decimalpoint());
			has_decimal = true;
			int l2 = 0;
			while(str[str.length() - 1 - l2] == '0') {
				l2++;
			}
			int decimals = str.length() - l10 - 1;
			if((!exact || approx) && po.show_ending_zeroes && (int) str.length() - precision - 1 < l2) {
				l2 = str.length() - precision - 1;
				if(po.use_max_decimals && po.max_decimals >= 0 && decimals - l2 > po.max_decimals) {
					l2 = decimals - po.max_decimals;
				}
			}
			if(l2 > 0 && !infinite_series) {
				if(min_decimals > 0 && (!approx || (!po.show_ending_zeroes && (int) str.length() - precision - 1 < l2))) {
					if(decimals - min_decimals < l2) l2 = decimals - min_decimals;
					if(approx && (int) str.length() - precision - 1 > l2) l2 = str.length() - precision - 1;
				}
				if(l2 > 0) str = str.substr(0, str.length() - l2);
			}
			if(str[str.length() - 1] == po.decimalpoint()[0]) {
				str.erase(str.end() - 1);
				has_decimal = false;
			}
		}
		int decimals = 0;
		if(l10 > 0) {
			decimals = str.length() - l10 - 1;
		}

		if(str.empty()) {
			str = "0";
		}
		if(!exact && str == "0" && po.show_ending_zeroes && po.use_max_decimals && po.max_decimals >= 0 && po.max_decimals < precision) {
			str += po.decimalpoint();
			for(; decimals < po.max_decimals; decimals++) str += '0';
		}
		if(exact && min_decimals > decimals) {
			if(decimals <= 0) {
				str += po.decimalpoint();
				decimals = 0;
				has_decimal = true;
			}
			for(; decimals < min_decimals; decimals++) {
				str += "0";
			}
		}
		if(str[str.length() - 1] == po.decimalpoint()[0]) {
			str.erase(str.end() - 1);
			has_decimal = false;
		}
		
		str = format_number_string(str, base, po.base_display, !ips.minus && neg, !has_decimal);
		
		if(infinite_series) {
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) ("…", po.can_display_unicode_string_arg))) str += "…";
			else str += "...";
		}
		if(expo != 0) {
			if(ips.iexp) *ips.iexp = expo;
			if(ips.exp) {
				if(ips.exp_minus) {
					*ips.exp_minus = expo < 0;
					if(expo < 0) expo = -expo;
				}
				*ips.exp = i2s(expo);
			} else {
				if(po.lower_case_e) str += "e";
				else str += "E";
				str += i2s(expo);
			}
		}
		if(ips.minus) *ips.minus = neg;
		if(ips.num) *ips.num = str;
	} else {
		Number num, den;
		num.setInternal(mpq_numref(r_value));
		den.setInternal(mpq_denref(r_value));
		if(isApproximate()) {
			num.setApproximate();
			den.setApproximate();
		}
		bool approximately_displayed = false;
		PrintOptions po2 = po;
		po2.is_approximate = &approximately_displayed;
		str = num.print(po2, ips);
		if(approximately_displayed && base != BASE_ROMAN_NUMERALS) {
			po2 = po;
			po2.number_fraction_format = FRACTION_DECIMAL;
			return print(po2, ips);
		}
		if(ips.num) *ips.num = str;
		if(po.spacious) str += " ";
		if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
			str += SIGN_DIVISION;
		} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
			str += SIGN_DIVISION_SLASH;
		} else {
			str += "/";
		}
		if(po.spacious) str += " ";
		InternalPrintStruct ips_n = ips;
		ips_n.minus = NULL;
		string str2 = den.print(po2, ips_n);
		if(approximately_displayed && base != BASE_ROMAN_NUMERALS) {
			po2 = po;
			po2.number_fraction_format = FRACTION_DECIMAL;
			return print(po2, ips);
		}
		if(ips.den) *ips.den = str2;
		str += str2;
		if(po.is_approximate && approximately_displayed) *po.is_approximate = true;
	}
	return str;
}


