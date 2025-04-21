int
main (int argc, char **argv)
{
  int limit;

  char *sieve;
  size_t size, i;

  struct prime *prime_list;
  unsigned nprimes;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s LIMIT\n"
               "Produces a list of odd primes <= LIMIT\n", argv[0]);
      return EXIT_FAILURE;
    }
  limit = atoi (argv[1]);
  if (limit < 3)
    return EXIT_SUCCESS;

  /* Make limit odd */
  if ( !(limit & 1))
    limit--;

  size = (limit-1)/2;
  /* sieve[i] represents 3+2*i */
  sieve = xalloc (size);
  memset (sieve, 1, size);

  prime_list = xalloc (size * sizeof (*prime_list));
  nprimes = 0;

  for (i = 0; i < size;)
    {
      unsigned p = 3+2*i;
      unsigned j;

      process_prime (&prime_list[nprimes++], p);

      for (j = (p*p - 3)/2; j < size; j+= p)
        sieve[j] = 0;

      <vul-start>while (i < size && sieve[++i] == 0)<vul-end>
        ;
    }

  output_primes (prime_list, nprimes);

  if (ferror (stdout) + fclose (stdout))
    {
      fprintf (stderr, "write error: %s\n", strerror (errno));
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
