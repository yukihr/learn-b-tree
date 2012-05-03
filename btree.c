#include <stdio.h>
#include <stdlib.h>

#define M 2

typedef int keytype;
typedef enum {FALSE, TRUE} boolean;

typedef struct page {
  int n; /* number of data */
  keytype key[2 * M]; /* key */
  struct page *branch[2 * M + 1]; /* pointer to other pages */
} *pageptr;

pageptr root = NULL; /* ROOT */
keytype key;
boolean done, deleted, undersize;
pageptr newp; /* new page returned by insert() */
char *message; /* message returned by functions */

/*
 * create new page
 */
pageptr newpage(void)
{
  pageptr p;

  if ((p = malloc(sizeof *p)) == NULL) {
    printf("Run out of memory..\n"); exit(EXIT_FAILURE);
  }
  return p;
}

/*
 * search `key' from B-tree
 */
void search(void)
{
  pageptr p;
  int k;

  p = root;
  while (p != NULL) {
    k = 0;
    while (k < p->n && p->key[k] < key) k++;
    if (k < p->n && p->key[k] == key) {
      message = "Found key!"; return;
    }
    p = p->branch[k];
  }
  message = "Couldn't find any keys..";
}


/*
 * Insertion
 */
// insert `key' into `p->key[k]'
void insertitem(pageptr p, int k)
{
  int i;

  // k の箇所までをひとつ右側にずらす
  for (i = p->n; i > k; i--) {
    p->key[i] = p->key[i - 1];
    p->branch[i + 1] = p->branch[i];
  }
  // kの箇所にkeyを挿入し、branchにpageをひとつ追加する
  p->key[k] = key; p->branch[k + 1] = newp; p->n++;
}

// insert `key' into `p->key[k]' and devide page `p' into 2 pages
void split(pageptr p, int k)
{
  int j, m;
  pageptr q; // splitted new page

  if (k <= M) m = M; else m = M + 1;
  q = newpage();
  for (j = m + 1; j <= 2 * M; j++) {
    q->key[j - m -1] = p->key[j - 1];
    q->branch[j - m] = p->branch[j];
  }
  q->n = 2 * M - m; p->n = m;

  if (k <= M) insertitem(p, k);
  else        insertitem(q, k - m);

  key = p->key[p->n - 1];
  q->branch[0] = p->branch[p->n]; p->n--;

  newp = q;
}

// insert sub walking tree recursively from page `p'
void insertsub(pageptr p)
{
  int k;

  if (p == NULL) {
    done = FALSE; newp = NULL; return;
  }

  k = 0;
  while (k < p->n && p->key[k] < key) k++;
  if (k < p->n && p->key[k] == key) {
    message = "Already inserted.."; done = TRUE;
    return;
  }
  insertsub(p->branch[k]);
  if (done) return;
  if (p->n < 2 * M) { // dont split
    insertitem(p, k); done = TRUE;
  } else { // split
    split(p, k); done = FALSE;
  }
}

// insert `key' to B-tree
void insert(void)
{
  pageptr p;

  message = "Inserted!";
  insertsub(root); if (done) return;
  p = newpage(); p->n = 1; p->key[0] = key;
  p->branch[0] = root; p->branch[1] = newp; root = p;
}


/*
 * Removal
 */
// remove `p->key[k]`, `p->branch[k+1]` and activate `undersize` when page is too small
void removeitem(pageptr p, int k)
{
  while (++k < p->n) {
    p->key[k-1] = p->key[k];
    p->branch[k] = p->branch[k+1];
  }
  undersize = --(p->n) < M;
}


/*
 * Moving
 */
// move most right element of `p->branch[k-1]` to `p->branch[k]` through p->key[k-1]
void moveright(pageptr p, int k)
{
  int j;
  pageptr left, right;

  left = p->branch[k-1]; right = p->branch[k];
  // まず入れる方に余裕を作る
  for (j = right->n; j > 0; j--) {
    right->key[j] = right->key[j - 1];
    right->branch[j + 1] = right->branch[j];
  }
  right->branch[1] = right->branch[0];
  right->n++;
  right->key[0] = p->key[k - 1];
  p->key[k - 1] = left->key[left->n - 1];
  right->branch[0] = left->branch[left->n];
  left->n--;
}

// move most left element of `p->branch[k]` to `p->branch[k-1]` through p->key[k-1]
void moveleft(pageptr p, int k)
{
  int j;
  pageptr left, right;

  left = p->branch[k-1]; right = p->branch[k];
  left->n++;
  left->key[left->n - 1] = p->key[k-1];
  left->branch[left->n] = right->branch[0];
  p->key[k - 1] = right->key[0];
  right->branch[0] = right->branch[1];
  right->n--;
  for (j = 1; j <= right->n; j++) {
    right->key[j - 1] = right->key[j];
    right->branch[j] = right->branch[j + 1];
  }
}

// combine p->branch[k-1] and p->branch[k]
void combine(pageptr p, int k)
{
  int j;
  pageptr left, right;

  right = p->branch[k];
  left = p->branch[k-1];
  left->n++;
  left->key[left->n - 1] = p-> key[k-1];
  left->branch[left->n] = right->branch[0];
  for (j = 1; j <= right->n; j++) {
    left->n++;
    left->key[left->n - 1] = right->key[j - 1];
    left->branch[left->n] = right->branch[j];
  }
  removeitem(p, k - 1);
  free(right);
}

// restore over-shrinked p->branch[k]
void restore(pageptr p, int k)
{
  undersize = FALSE;
  if (k > 0) {
    if (p->branch[k - 1]->n > M) moveright(p, k);
    else combine(p, k);
  } else {
    if (p->branch[1]->n > M) moveleft(p, 1);
    else combine(p, 1);
  }
}


/*
 * Deletion
 */
// remove `key` walking tree recursively from page `p`
void deletesub(pageptr p)
{
  int k;
  pageptr q;

  if (p == NULL) return; // not found..
  k = 0;
  while (k < p->n && p->key[k] < key) k++;
  if (k < p->n && p->key[k] == key) { // found!
    deleted = TRUE;
    if ((q = p->branch[k + 1]) != NULL) {
      while (q->branch[0] != NULL) q = q->branch[0];
      p->key[k] = key = q->key[0];
      deletesub(p->branch[k + 1]);
      if (undersize) restore(p, k + 1);
    } else removeitem(p, k);
  } else {
    deletesub(p->branch[k]);
    if (undersize) restore(p, k);
  }
}

// remove `key` from B-tree
void delete(void)
{
  pageptr p;

  deleted = undersize = FALSE;
  deletesub(root);
  if (deleted) {
    if (root->n == 0) { // when root contains no keys
      p = root; root = root->branch[0]; free(p);
    }
    message = "Deleted!";
  } else message = "Couldn't find key on the tree..";
}


// UI
// print for demo
void printtree(pageptr p)
{
  static int depth = 0;
  int k;

  if (p == NULL) { printf("."); return; }
  printf("("); depth++;
  for (k = 0; k < p->n; k++) {
    printtree(p->branch[k]); // rec
    printf("%d", p->key[k]);
  }
  printtree(p->branch[p->n]); // rec
  printf(")"); depth--;
}

#include <ctype.h>

int main()
{
  char s[2];

  for ( ; ; ) {
    printf("[I]nsert, [S]earch, [D]elete (n:integer) ? ");
    if (scanf("%1s%d", s, &key) != 2) break;
    switch (s[0]) {
    case 'I': case 'i': insert(); break;
    case 'S': case 's': search(); break;
    case 'D': case 'd': delete(); break;
    default : message = "???"; break;
    }
    printf("%s\n\n", message);
    printtree(root); printf("\n\n");
  }

  return EXIT_SUCCESS;
}
