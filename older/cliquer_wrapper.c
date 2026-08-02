#include <stdint.h>
#include "nautycliquer.h"

int cliquer_find_target(int n, const uint64_t *adjacency, int words,
                        const uint64_t *candidate, int target, int *out) {
  graph_t *g = graph_new(n);
  if (!g) return -1;
  for (int i = 0; i < n; ++i) {
    if (!((candidate[i >> 6] >> (i & 63)) & 1)) continue;
    for (int j = i + 1; j < n; ++j) {
      if (!((candidate[j >> 6] >> (j & 63)) & 1)) continue;
      if ((adjacency[i * words + (j >> 6)] >> (j & 63)) & 1)
        GRAPH_ADD_EDGE(g, i, j);
    }
  }
  set_t clique = clique_unweighted_find_single(g, target, target, FALSE, NULL);
  if (!clique) {
    graph_free(g);
    return 0;
  }
  int size = 0;
  for (int v = 0; v < n; ++v)
    if (SET_CONTAINS_FAST(clique, v)) out[size++] = v;
  set_free(clique);
  graph_free(g);
  return size;
}

typedef int (*cliquer_target_callback)(int, const int *, void *);

struct enumeration_data {
  cliquer_target_callback callback;
  void *user_data;
};

static boolean forward_clique(set_t clique, graph_t *g,
                              clique_options *options) {
  struct enumeration_data *data = options->user_data;
  int vertices[256];
  int size = 0;
  for (int v = 0; v < g->n; ++v)
    if (SET_CONTAINS_FAST(clique, v)) vertices[size++] = v;
  return data->callback(size, vertices, data->user_data) ? TRUE : FALSE;
}

int cliquer_enumerate_target(int n, const uint64_t *adjacency, int words,
                             const uint64_t *candidate, int target,
                             cliquer_target_callback callback,
                             void *user_data) {
  graph_t *g = graph_new(n);
  if (!g) return -1;
  for (int i = 0; i < n; ++i) {
    if (!((candidate[i >> 6] >> (i & 63)) & 1)) continue;
    for (int j = i + 1; j < n; ++j) {
      if (!((candidate[j >> 6] >> (j & 63)) & 1)) continue;
      if ((adjacency[i * words + (j >> 6)] >> (j & 63)) & 1)
        GRAPH_ADD_EDGE(g, i, j);
    }
  }
  struct enumeration_data data = {callback, user_data};
  clique_options options = *clique_default_options;
  options.user_function = forward_clique;
  options.user_data = &data;
  options.clique_list = NULL;
  options.clique_list_length = 0;
  int count = clique_unweighted_find_all(g, target, target, FALSE, &options);
  graph_free(g);
  return count;
}
