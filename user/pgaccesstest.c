#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "user/user.h"

static const unsigned int readonly_mask;

static void
fail(const char *message)
{
  printf("pgaccesstest: FAIL: %s\n", message);
  exit(1);
}

static void
check_mask(const char *name, unsigned int got, unsigned int want)
{
  if(got != want){
    printf("pgaccesstest: %s: got 0x%x, expected 0x%x\n",
           name, got, want);
    exit(1);
  }
}

static void
expect_error(const char *name, void *base, int npages,
             unsigned int *mask)
{
  if(pgaccess(base, npages, mask) != -1){
    printf("pgaccesstest: %s: expected -1\n", name);
    exit(1);
  }
}

static void
touch_page(volatile char *region, int page)
{
  volatile char *p = region + (uint64)page * PGSIZE;
  char value = *p;
  *p = value + 1;
}

static void
clear_accessed(volatile char *region, unsigned int *mask)
{
  if(pgaccess((void *)(region + 17), 32, mask) != 0)
    fail("could not clear initial access bits");
}

int
main(void)
{
  char *raw = sbrk(33 * PGSIZE);
  if(raw == (char *)-1)
    fail("sbrk failed");

  // Align up; the 33rd allocated page covers the alignment gap.
  volatile char *region = (volatile char *)PGROUNDUP((uint64)raw);
  unsigned int mask = 0;

  // Selected middle pages, using a deliberately unaligned base.
  clear_accessed(region, &mask);
  touch_page(region, 1);
  touch_page(region, 2);
  touch_page(region, 30);
  if(pgaccess((void *)(region + 17), 32, &mask) != 0)
    fail("valid 32-page query failed");
  check_mask("pages 1, 2, 30", mask,
             (1U << 1) | (1U << 2) | (1U << 30));

  // The previous call cleared those bits; with no new touches the mask is 0.
  if(pgaccess((void *)region, 32, &mask) != 0)
    fail("repeat query failed");
  check_mask("no new access", mask, 0);

  // A touch after clearing must be reported again.
  touch_page(region, 2);
  if(pgaccess((void *)region, 32, &mask) != 0)
    fail("query after repeat touch failed");
  check_mask("page 2 touched again", mask, 1U << 2);

  clear_accessed(region, &mask);
  touch_page(region, 0);
  if(pgaccess((void *)region, 32, &mask) != 0)
    fail("page 0 query failed");
  check_mask("page 0", mask, 1U << 0);

  clear_accessed(region, &mask);
  touch_page(region, 31);
  if(pgaccess((void *)region, 32, &mask) != 0)
    fail("page 31 query failed");
  check_mask("page 31", mask, 1U << 31);

  // A one-page query reports its first page as bit zero, even when base is
  // unaligned within that page.
  clear_accessed(region, &mask);
  touch_page(region, 5);
  if(pgaccess((void *)(region + 5 * PGSIZE + 19), 1, &mask) != 0)
    fail("valid one-page query failed");
  check_mask("one-page query", mask, 1U << 0);

  // Choose a page after the allocated range; round up to skip its last
  // partially allocated page, if any.
  uint64 hole = PGROUNDUP((uint64)raw + 33 * PGSIZE);

  expect_error("zero pages", (void *)region, 0, &mask);
  expect_error("negative pages", (void *)region, -1, &mask);
  expect_error("33 pages", (void *)region, 33, &mask);
  expect_error("base at MAXVA", (void *)MAXVA, 1, &mask);
  expect_error("range crosses MAXVA", (void *)(MAXVA - 1), 2, &mask);
  expect_error("base address wraps", (void *)(~(uint64)0), 1, &mask);
  expect_error("unmapped measured page", (void *)hole, 1, &mask);

  // exec places the inaccessible stack guard immediately below the stack.
  uint64 stack_marker;
  uint64 stack_page = PGROUNDDOWN((uint64)&stack_marker);
  expect_error("stack guard", (void *)(stack_page - PGSIZE), 1, &mask);
  expect_error("TRAPFRAME", (void *)TRAPFRAME, 1, &mask);
  expect_error("TRAMPOLINE", (void *)TRAMPOLINE, 1, &mask);

  // The literal is in a read-only user segment.
  expect_error("read-only output buffer", (void *)region, 1,
               (unsigned int *)(uint64)&readonly_mask);
  expect_error("unmapped output buffer", (void *)region, 1,
               (unsigned int *)hole);

  // Grow to a page boundary. The next page is unmapped, so this 4-byte
  // output buffer straddles a writable page and an unmapped page.
  uint64 brk = (uint64)sbrk(0);
  uint64 boundary = PGROUNDUP(brk);
  if(boundary == brk)
    boundary += PGSIZE;
  if(sbrk(boundary - brk) == (char *)-1)
    fail("could not prepare cross-page output test");
  expect_error("output crosses into unmapped page", (void *)region, 1,
               (unsigned int *)(boundary - 2));

  // A valid query after all failures confirms the syscall remains usable.
  if(pgaccess((void *)region, 32, &mask) != 0)
    fail("valid query after error cases failed");

  printf("pgaccesstest: OK\n");
  exit(0);
}
