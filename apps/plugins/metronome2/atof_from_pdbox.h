#ifndef METRONOME2_ATOF_H
#define METRONOME2_ATOF_H

/* This is a nasty hack: Ripped out atof() from pdbox plugin,
   Copyright (C) 2009, 2010 Wincent Balin */

/* Implementation of strtod() and atof(),
   taken from SanOS (http://www.jbox.dk/sanos/). */
static int rb_errno = 0;

double rb_strtod(const char *str, char **endptr)
{
  double number;
  int exponent;
  int negative;
  char *p = (char *) str;
  double p10;
  int n;
  int num_digits;
  int num_decimals;

    /* Reset Rockbox errno -- W.B. */
#ifdef ROCKBOX
    rb_errno = 0;
#endif

  // Skip leading whitespace
  while (isspace(*p)) p++;

  // Handle optional sign
  negative = 0;
  switch (*p) 
  {
    case '-': negative = 1; // Fall through to increment position
    case '+': p++;
  }

  number = 0.;
  exponent = 0;
  num_digits = 0;
  num_decimals = 0;

  // Process string of digits
  while (isdigit(*p))
  {
    number = number * 10. + (*p - '0');
    p++;
    num_digits++;
  }

  // Process decimal part
  if (*p == '.') 
  {
    p++;

    while (isdigit(*p))
    {
      number = number * 10. + (*p - '0');
      p++;
      num_digits++;
      num_decimals++;
    }

    exponent -= num_decimals;
  }

  if (num_digits == 0)
  {
#ifdef ROCKBOX
    rb_errno = 1;
#else
    errno = ERANGE;
#endif
    return 0.0;
  }

  // Correct for sign
  if (negative) number = -number;

  // Process an exponent string
  if (*p == 'e' || *p == 'E') 
  {
    // Handle optional sign
    negative = 0;
    switch(*++p) 
    {   
      case '-': negative = 1;   // Fall through to increment pos
      case '+': p++;
    }

    // Process string of digits
    n = 0;
    while (isdigit(*p)) 
    {   
      n = n * 10 + (*p - '0');
      p++;
    }

    if (negative) 
      exponent -= n;
    else
      exponent += n;
  }

#ifndef ROCKBOX
  if (exponent < DBL_MIN_EXP  || exponent > DBL_MAX_EXP)
  {
    errno = ERANGE;
    return HUGE_VAL;
  }
#endif

  // Scale the result
  p10 = 10.;
  n = exponent;
  if (n < 0) n = -n;
  while (n) 
  {
    if (n & 1) 
    {
      if (exponent < 0)
        number /= p10;
      else
        number *= p10;
    }
    n >>= 1;
    p10 *= p10;
  }

#ifndef ROCKBOX
  if (number == HUGE_VAL) errno = ERANGE;
#endif
  if (endptr) *endptr = p;

  return number;
}

double rb_atof(const char *str)
{
    return rb_strtod(str, NULL);
}

#endif
