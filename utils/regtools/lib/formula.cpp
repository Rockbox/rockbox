#include "soc_desc.hpp"
#include <cstdarg>
#include <cstdio>

using namespace soc_desc;

namespace soc_desc
{

namespace
{

struct formula_evaluator
{
    std::string formula;
    size_t pos;
    error_context_t& ctx;
    std::string m_loc;

    bool err(const char *fmt, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer,sizeof(buffer), fmt, args);
        va_end(args);
        ctx.add(err_t(err_t::FATAL, m_loc, buffer));
        return false;
    }

    formula_evaluator(const std::string& s, error_context_t& ctx):pos(0),ctx(ctx)
    {
        for(size_t i = 0; i < s.size(); i++)
            if(!isspace(s[i]))
                formula.push_back(s[i]);
    }

    void set_location(const std::string& loc)
    {
        m_loc = loc;
    }

    void adv()
    {
        pos++;
    }

    char cur()
    {
        return end() ? 0 : formula[pos];
    }

    bool end()
    {
        return pos >= formula.size();
    }

    bool parse_digit(char c, int basis, soc_word_t& res)
    {
        c = tolower(c);
        if(isdigit(c))
        {
            res = c - '0';
            return true;
        }
        if(basis == 16 && isxdigit(c))
        {
            res = c + 10 - 'a';
            return true;
        }
        return false;
    }

    bool parse_signed(soc_word_t& res)
    {
        char op = cur();
        if(op == '+' || op == '-')
        {
            adv();
            if(!parse_signed(res))
                return false;
            if(op == '-')
                res *= -1;
            return true;
        }
        else if(op == '(')
        {
            adv();
            if(!parse_expression(res))
                return false;
            if(cur() != ')')
                return err("expected ')', got '%c'", cur());
            adv();
            return true;
        }
        else if(isdigit(op))
        {
            res = op - '0';
            adv();
            int basis = 10;
            if(op == '0' && cur() == 'x')
            {
                basis = 16;
                adv();
            }
            soc_word_t digit = 0;
            while(parse_digit(cur(), basis, digit))
            {
                res = res * basis + digit;
                adv();
            }
            return true;
        }
        else if(isalpha(op) || op == '_')
        {
            std::string name;
            while(isalnum(cur()) || cur() == '_')
            {
                name.push_back(cur());
                adv();
            }
            return get_variable(name, res);
        }
        else
            return err("express signed expression, got '%c'", op);
    }

    bool parse_term(soc_word_t& res)
    {
        if(!parse_signed(res))
            return false;
        while(cur() == '*' || cur() == '/' || cur() == '%')
        {
            char op = cur();
            adv();
            soc_word_t tmp;
            if(!parse_signed(tmp))
                return false;
            if(op == '*')
                res *= tmp;
            else if(tmp != 0)
                res = op == '/' ? res / tmp : res % tmp;
            else
                return err("division by 0");
        }
        return true;
    }

    bool parse_expression(soc_word_t& res)
    {
        if(!parse_term(res))
            return false;
        while(!end() && (cur() == '+' || cur() == '-'))
        {
            char op = cur();
            adv();
            soc_word_t tmp;
            if(!parse_term(tmp))
                return false;
            if(op == '+')
                res += tmp;
            else
                res -= tmp;
        }
        return true;
    }

    bool parse(soc_word_t& res)
    {
        bool ok = parse_expression(res);
        if(ok && !end())
            err("unexpected character '%c'", cur());
        return ok && end();
    }

    virtual bool get_variable(std::string name, soc_word_t& res)
    {
        return err("unknown variable '%s'", name.c_str());
    }
};

struct my_evaluator : public formula_evaluator
{
    const std::map< std::string, soc_word_t>& var;

    my_evaluator(const std::string& formula, const std::map< std::string, soc_word_t>& _var,
        error_context_t& ctx)
        :formula_evaluator(formula, ctx), var(_var) {}

    virtual bool get_variable(std::string name, soc_word_t& res)
    {
        std::map< std::string, soc_word_t>::const_iterator it = var.find(name);
        if(it == var.end())
            return formula_evaluator::get_variable(name, res);
        else
        {
            res = it->second;
            return true;
        }
    }
};

}

bool evaluate_formula(const std::string& formula,
    const std::map< std::string, soc_word_t>& var, soc_word_t& result,
    const std::string& loc, error_context_t& error)
{
    my_evaluator e(formula, var, error);
    e.set_location(loc);
    return e.parse(result);
}

} // soc_desc
