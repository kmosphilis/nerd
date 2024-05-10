#include <prb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "nerd_utils.h"
#include "queue.h"
#include "rule_hypergraph.h"

typedef struct Vertex Vertex;

typedef struct Edge {
  Rule *rule;
  Vertex **from;
  size_t number_of_vertices;
} Edge;

typedef struct Vertex {
  Literal *literal;
  Edge **edges;
  size_t number_of_edges;
} Vertex;

typedef struct prb_table prb_table;

struct RuleHyperGraph {
  prb_table *literal_tree;
  bool use_backward_chaining;
};

// XXX Should we remove literals that are not used?
// To do that we will need an additional field which will hold the vertices that
// a vertex is being used because if we delete a vertex being used, we will run
// into segmentation faults.

/**
 * @brief Destructs the given Edge and the Rule that resides within.
 *
 * @param edge The Edge to be destructed. It should be reference to an Edge *
 * (Edge ** - a pointer to an Edge *). Upon succession, this parameter will
 * become NULL.
 */
void edge_destructor(Edge **const edge) {
  if (edge && *edge) {
    safe_free((*edge)->from);
    rule_destructor(&((*edge)->rule));
    safe_free(*edge);
  }
}

/**
 * @brief Constructs a RuleHyperGraph Vertex.
 *
 * @param literal The Literal which the Vertex will be build for.
 *
 * @return A new Vertex *. Use vertex_destructor to deallocate it.
 */
Vertex *vertex_constructor(Literal *const literal) {
  Vertex *vertex = (Vertex *)malloc(sizeof(Vertex));
  vertex->literal = literal;
  vertex->edges = NULL;
  vertex->number_of_edges = 0;
  return vertex;
}

/**
 * @brief Destructs the given Vertex.
 *
 * @param vertex The Vertex to be destructed. It should be reference to a Vertex
 * * (Vertex ** - a pointer to a Vertex *). Upon succession, this parameter will
 * become NULL.
 * @param destruct_literal If true is given, the Literal within the vertrex will
 * be destructed, and if false is given it will not.
 */
void vertex_destructor(Vertex **const vertex, const bool destruct_literal) {
  if (vertex && *vertex) {
    unsigned int i;
    for (i = 0; i < (*vertex)->number_of_edges; ++i) {
      edge_destructor(&((*vertex)->edges[i]));
    }
    safe_free((*vertex)->edges);
    if (destruct_literal) {
      literal_destructor(&((*vertex)->literal));
    }
    safe_free(*vertex);
  }
}

/**
 * @brief Constructs a RuleHyperGraph Edge.
 *
 * @param rule_hypergraph The RuleHyperGraph to find the corresponding Vertices
 * to be connected.
 * @param rule The Rule to show which Literals should be connected. The Body of
 * a rule has the origin Vertices, and the head has the destination Vertex. It
 * should be reference to a Rule * (Rule ** - a pointer to a Rule *). If the
 * given Rule had took ownership of its content, a new Rule will be created
 * without taking the ownership of that content as it will belong to the
 * internal RB-Tree. If it does not take the ownership of the its content, the
 * content pointers might stil change, as the body and head will take the
 * pointer of the item allocated in the RB-Tree.
 * @param head_vertex The Vertex that the edge will be created for. It will not
 * be added to the Vertex.
 *
 * @return A new Edge *. Use edge_destructor to deallocate it.
 */
Edge *edge_constructor(RuleHyperGraph *const rule_hypergraph, Rule **const rule,
                       Vertex *const head_vertex) {
  if (!rule) {
    return NULL;
  }

  Edge *edge = (Edge *)malloc(sizeof(Edge));
  edge->from = (Vertex **)malloc(sizeof(Vertex *) * (*rule)->body->size);
  edge->number_of_vertices = (*rule)->body->size;
  Vertex *vertex;
  void *result;
  unsigned int i;

  if (rule_took_ownership(*rule)) {
    Literal **body = (*rule)->body->literals;
    for (i = 0; i < (*rule)->body->size; ++i) {
      vertex = vertex_constructor((*rule)->body->literals[i]);

      result = prb_insert(rule_hypergraph->literal_tree, vertex);
      if (result) {
        vertex_destructor(&vertex, true);
        vertex = result;
        body[i] = vertex->literal;
      }
      edge->from[i] = vertex;
    }
    Rule *new_rule =
        rule_constructor((*rule)->body->size, body, &(head_vertex->literal),
                         (*rule)->weight, false);

    if ((*rule)->head != head_vertex->literal) {
      literal_destructor(&((*rule)->head));
    }
    free(body);
    safe_free((*rule)->body);
    safe_free((*rule));
    *rule = new_rule;
  } else {
    (*rule)->head = head_vertex->literal;
    for (i = 0; i < (*rule)->body->size; ++i) {
      vertex = vertex_constructor((*rule)->body->literals[i]);

      result = prb_insert(rule_hypergraph->literal_tree, vertex);
      if (result) {
        vertex_destructor(&vertex, false);
        vertex = result;
        (*rule)->body->literals[i] = vertex->literal;
      }
      edge->from[i] = vertex;
    }
  }
  edge->rule = *rule;
  return edge;
}

/**
 * @brief Adds an Edge to a Vertex. The Vertex is the head of the Rule, and the
 * Edge contains the origin (body) of the Rule.
 *
 * @param vertex The Vertex to add the Edge to.
 * @param edge The Edge to be added to the Vertex.
 */
void vertex_add_edge(Vertex *const vertex, Edge *const edge) {
  if (vertex && edge) {
    vertex->edges = (Edge **)realloc(
        vertex->edges, sizeof(Edge *) * ++vertex->number_of_edges);
    vertex->edges[vertex->number_of_edges - 1] = edge;
  }
}

/**
 * @brief Removes an Edge from a Vertex.
 *
 * @param vertex The Vertex to remove the Edge from.
 * @param index The index of the Edge to be removed. If >
 * vertex.number_of_edges, it will fail.
 */
void vertex_remove_edge(Vertex *const vertex, unsigned int index) {
  if (vertex && (index < vertex->number_of_edges)) {
    edge_destructor(&(vertex->edges[index]));
    --vertex->number_of_edges;
    if (vertex->number_of_edges == 0) {
      safe_free(vertex->edges);
      return;
    }

    Edge **old = vertex->edges;
    vertex->edges = (Edge **)malloc(sizeof(Edge *) * vertex->number_of_edges);

    if (index == 0) {
      memcpy(vertex->edges, old + 1, sizeof(Edge *) * vertex->number_of_edges);
    } else if (index == vertex->number_of_edges) {
      memcpy(vertex->edges, old, sizeof(Edge *) * vertex->number_of_edges);
    } else {
      memcpy(vertex->edges, old, sizeof(Edge *) * index);
      memcpy(vertex->edges + index, old + index + 1,
             sizeof(Edge *) * (vertex->number_of_edges - index));
    }

    free(old);
  }
}

/**
 * @brief Function to be used with the RB-Tree to deallocate the Vertices and
 * the Edges.
 */
void item_destructor(void *item, void *param) {
  if (param)
    return;

  Vertex *v = (Vertex *)item;
  vertex_destructor(&v, true);
}

/**
 * @brief Function to be used with the RB-Tree to find the appropriate location
 * of a Vertex through comparison.
 */
int compare_literals(const void *vertex1, const void *vertex2, void *param) {
  if (param)
    return 0;

  const Vertex *v1 = (Vertex *)vertex1, *v2 = (Vertex *)vertex2;
  char *l1_string = literal_to_string(v1->literal),
       *l2_string = literal_to_string(v2->literal);
  int result = strcmp(l1_string, l2_string);

  free(l1_string);
  free(l2_string);

  return result;
}

/**
 * @brief Costructs an empty RuleHyperGraph with an empty RB-Tree.
 *
 * @param use_backward_chaining A boolean value which indicates whether the
 * hypergraph should demoted rules using the backward chaining algorithm or not.
 *
 * @return A new RuleHyperGraph *. Use rule_hypergraph_destructor to deallocate.
 */
RuleHyperGraph *
rule_hypergraph_empty_constructor(const bool use_backward_chaining) {
  RuleHyperGraph *hypergraph = (RuleHyperGraph *)malloc(sizeof(RuleHyperGraph));

  hypergraph->literal_tree = prb_create(compare_literals, NULL, NULL);
  hypergraph->use_backward_chaining = use_backward_chaining;

  return hypergraph;
}

/**
 * @brief Destructs the given RuleHyperGraph and its RB-Tree.
 *
 * @param rule_hypergraph The RuleHyperGraph to be destructed. It should be
 * reference to a RuleHyperGraph (RuleHyperGraph ** - a pointer to a
 * RuleHyperGraph *). Upon succession, this parameter will become NULL.
 */
void rule_hypergraph_destructor(RuleHyperGraph **const rule_hypergraph) {
  if (rule_hypergraph && *rule_hypergraph && (*rule_hypergraph)->literal_tree) {
    prb_destroy((*rule_hypergraph)->literal_tree, item_destructor);
    safe_free(*rule_hypergraph);
  }
}

/**
 * @brief Makes a copy of the RuleHyperGraph in the given KnowledgeBase. If any
 * of the parameters is NULL, the process will fail.
 *
 * @param destination The KnowledgeBase to save the copy. It should be a
 * reference to the struct's pointer (to a KnowledgeBase * - &(KnowledgeBase
 * *)).
 * @param source The KnowledgeBase to be copied.
 */
void rule_hypergraph_copy(KnowledgeBase **const destination,
                          const KnowledgeBase *const source) {
  if (destination && source) {
    (*destination)->hypergraph = rule_hypergraph_empty_constructor(
        source->hypergraph->use_backward_chaining);

    struct prb_traverser traverser;

    Vertex *current_vertex =
        prb_t_first(&traverser, source->hypergraph->literal_tree);

    unsigned int i, j;
    while (current_vertex) {
      for (i = 0; i < current_vertex->number_of_edges; ++i) {
        Literal *head,
            **body = (Literal **)malloc(
                sizeof(Literal *) * current_vertex->edges[i]->rule->body->size);
        literal_copy(&head, current_vertex->edges[i]->rule->head);
        for (j = 0; j < current_vertex->edges[i]->rule->body->size; ++j) {
          literal_copy(&(body[j]),
                       current_vertex->edges[i]->rule->body->literals[j]);
        }
        Rule *rule = rule_constructor(
            j, body, &head, current_vertex->edges[i]->rule->weight, true);
        free(body);

        knowledge_base_add_rule(*destination, &rule);
      }
      current_vertex = prb_t_next(&traverser);
    }
  }
}

/**
 * @brief Adds a Rule to the given RuleHyperGraph. This process creates the
 * appropriate Vertices and an Edge to connected them.
 *
 * @param rule_hypergraph The RuleHyperGraph to add the Rule.
 * @param rule The Rule to be added to the RuleHyperGraph. If the given Rule had
 * took ownership of its content, a new Rule will be created without taking the
 * ownership of that content as it will belong to the internal RB-Tree. If it
 * does not take the ownership of the its content, the content pointers might
 * stil change, as the body and head will take the pointer of the item allocated
 * in the RB-Tree.
 *
 * @return 1 if Rule was successfully added, 0 if it was not, and -1 if one of
 * the parameters was NULL.
 */
int rule_hypergraph_add_rule(RuleHyperGraph *const rule_hypergraph,
                             Rule **const rule) {
  if (rule_hypergraph && rule && *rule) {
    Vertex *head_vertex = vertex_constructor((*rule)->head);
    void *result = prb_insert(rule_hypergraph->literal_tree, head_vertex);

    if (result) {
      vertex_destructor(&head_vertex, false);
      head_vertex = result;
    }

    unsigned int i;
    for (i = 0; i < head_vertex->number_of_edges; ++i) {
      if (rule_equals(head_vertex->edges[i]->rule, *rule)) {
        return 0;
      }
    }
    Edge *edge = edge_constructor(rule_hypergraph, rule, head_vertex);
    vertex_add_edge(head_vertex, edge);
    return 1;
  }
  return -1;
}

/**
 * @brief Removes a Rule from the given RuleHyperGraph. This process only
 * deletes the connecting Edges, but leaves the Vertices involved unaffected
 * (could change in the future).
 *
 * @param rule_hypergraph The RuleHyperGraph to remove the Rule from.
 * @param rule The Rule to be removed.
 */
void rule_hypergraph_remove_rule(RuleHyperGraph *const rule_hypergraph,
                                 Rule *const rule) {
  if (rule_hypergraph && rule) {
    Vertex *v = vertex_constructor(rule->head);
    void *result = prb_find(rule_hypergraph->literal_tree, v);

    vertex_destructor(&v, false);
    if (!result) {
      return;
    }
    v = result;

    unsigned int i;
    for (i = 0; i < v->number_of_edges; ++i) {
      if (v->edges[i]->rule == rule) {
        vertex_remove_edge(v, i);
      }
    }
  }
}

/**
 * @brief Finds all the inactive Rules in the RuleHyperGraph inside the
 * KnowledgeBase.
 *
 * @param knowledge_base The KnowledgeBase which has the RuleHyperGraph to find
 * the inactive Rules from.
 * @param inactive_rules The output of the function to be returned. If NULL, the
 * function will not be performed. It should be a reference to the struct's
 * pointer (RuleQueue **). Make sure that the given double pointer is not
 * already allocated, otherwise its contents will be lost in the memory. The
 * RuleQueue which will be created will not have ownership of its Rules.
 */
void rule_hypergraph_get_inactive_rules(
    const KnowledgeBase *const knowledge_base,
    RuleQueue **const inactive_rules) {
  if (knowledge_base && inactive_rules) {
    *inactive_rules = rule_queue_constructor(false);
    struct prb_traverser traverser;
    Vertex *result = (Vertex *)prb_t_first(
        &traverser, knowledge_base->hypergraph->literal_tree);
    unsigned int i;
    while (result) {
      for (i = 0; i < result->number_of_edges; ++i) {
        Rule *current_rule = result->edges[i]->rule;

        if (current_rule->weight < knowledge_base->activation_threshold) {
          rule_queue_enqueue(*inactive_rules, &(current_rule));
        }
      }
      result = (Vertex *)prb_t_next(&traverser);
    }
  }
}

/**
 * @brief Updates the weight of (Promotes or Demotes) each rule according to the
 * given observation and inference.
 *
 * Currently backward chaining demotion is the only option.
 *
 * @param knowledge_base The KnowledgeBase that contains the rules to be
 * updated.
 * @param observation A Scene that contains the observed Literals.
 * @param inference A Scene that contains the inferred Literal that were made
 * by an inference engine.
 * @param promotion_rate The rate a rule should be promoted.
 * @param demotion_rate The rate a rule should be demoted.
 * @param increasing_demotion A boolean value which indicates whether the
 * chaining demotion should be increasing or not. It only works if and only if
 * backward chaining demotion is enabled.
 * @param header (Optional) A char ** containing the header of a file to compare
 * attributes. The rest two optional parameters should be given together.
 * @param header_size (Optional) The size_t of the header.
 * @param incompatibilities (Optional) A Scene ** containing the incompatible
 * literals corresponding to each header. It should have the same size, even if
 * no incompatible literals are given.
 */
void rule_hypergraph_update_rules(
    KnowledgeBase *const knowledge_base, const Scene *const observation,
    const Scene *const inference, const float promotion_rate,
    const float demotion_rate, const bool increasing_demotion, char **header,
    const size_t header_size, Scene **incompatibilities) {
  if (!(knowledge_base && observation && inference)) {
    return;
  }

  if (!((header && (header_size > 0) && incompatibilities) ||
        ((!header) && (header_size == 0) && (!incompatibilities)))) {
    return;
  }

  unsigned int i, j, k;
  Vertex *vertex_to_find, *current_vertex;
  Scene *observed_and_inferred, *observed_diff_inferred,
      *opposing_literals = scene_constructor(true);
  Rule *current_rule;

  scene_union(observation, inference, &observed_and_inferred);
  scene_difference(observation, inference, &observed_diff_inferred);
  for (i = 0; i < header_size; ++i) {
    for (j = 0; j < observed_diff_inferred->size; ++j) {
      if (strstr(observed_diff_inferred->literals[j]->atom, header[i])) {
        for (k = 0; k < incompatibilities[i]->size; ++k) {
          Literal *current_literal = incompatibilities[i]->literals[k], *copy;

          if (literal_equals(current_literal,
                             observed_diff_inferred->literals[j]) == 0) {
            literal_copy(&copy, current_literal);
            scene_add_literal(opposing_literals, &copy);
          }
        }
      }
    }
  }
  for (i = 0; i < observed_diff_inferred->size; ++i) {
    Literal *copy;
    literal_copy(&copy, observed_diff_inferred->literals[i]);
    literal_negate(copy);
    scene_add_literal(opposing_literals, &copy);
  }
  Scene *temp = observed_and_inferred;
  scene_difference(temp, opposing_literals, &observed_and_inferred);
  scene_destructor(&temp);
  scene_destructor(&observed_diff_inferred);

  // Finds all the Rules that concur by finding the observed Literal in the
  // RB-tree.
  for (i = 0; i < observation->size; ++i) {
    vertex_to_find = vertex_constructor(observation->literals[i]);
    current_vertex =
        prb_find(knowledge_base->hypergraph->literal_tree, vertex_to_find);
    vertex_destructor(&vertex_to_find, false);

    if (current_vertex && (current_vertex->number_of_edges != 0)) {
      for (j = 0; j < current_vertex->number_of_edges; ++j) {
        current_rule = current_vertex->edges[j]->rule;
        // Checks if the Rule is applicable given the union of observed and
        // inferred Literals.
        if (rule_applicable(current_rule, observed_and_inferred)) {
          bool is_inactive =
              current_rule->weight < knowledge_base->activation_threshold;
          current_rule->weight += promotion_rate;

          if (is_inactive &&
              (current_rule->weight >= knowledge_base->activation_threshold)) {
            rule_queue_enqueue(knowledge_base->active, &current_rule);
          }
        }
      }
    }
  }

  for (i = 0; i < opposing_literals->size; ++i) {
    vertex_to_find = vertex_constructor(opposing_literals->literals[i]);
    current_vertex =
        prb_find(knowledge_base->hypergraph->literal_tree, vertex_to_find);
    vertex_destructor(&vertex_to_find, false);

    if (current_vertex && (current_vertex->number_of_edges != 0)) {
      if (!knowledge_base->hypergraph->use_backward_chaining) {
        for (j = 0; j < current_vertex->number_of_edges; ++j) {
          current_rule = current_vertex->edges[j]->rule;
          // Checks if the Rule is applicable given the inferred and observed
          // Literals.
          bool was_active =
              current_rule->weight >= knowledge_base->activation_threshold;
          if (rule_applicable(current_rule, observed_and_inferred)) {
            current_rule->weight -= demotion_rate;

            if (was_active &&
                (current_rule->weight < knowledge_base->activation_threshold)) {
              rule_queue_remove_rule(
                  knowledge_base->active,
                  rule_queue_find(knowledge_base->active, current_rule), NULL);
            }
            if (current_rule->weight <= 0) {
              vertex_remove_edge(current_vertex, j);
              --j;
            }
          }
        }
        goto finished;
      }

      Queue *_depths, *_parent_vertex, *_vertices_to_check;
      queue_constructor(&_depths, sizeof(int), NULL);
      queue_constructor(&_parent_vertex, sizeof(Vertex *), NULL);
      queue_constructor(&_vertices_to_check, sizeof(Vertex *), NULL);
      queue_push_back(_depths, (void *)1);
      queue_push_back(_parent_vertex, NULL);
      queue_push_back(_vertices_to_check, (void *)current_vertex);

      do {
        current_vertex = (Vertex *)_vertices_to_check->front->data;
        for (j = 0; j < current_vertex->number_of_edges; ++j) {
          current_rule = current_vertex->edges[j]->rule;
          bool was_active =
              current_rule->weight >= knowledge_base->activation_threshold;
          if (rule_applicable(current_rule, observed_and_inferred)) {
            current_rule->weight -=
                demotion_rate * (increasing_demotion
                                     ? (int)(long)_depths->front->data
                                     : 1.0 / (int)(long)_depths->front->data);

            if (was_active &&
                (current_rule->weight < knowledge_base->activation_threshold)) {
              rule_queue_remove_rule(
                  knowledge_base->active,
                  rule_queue_find(knowledge_base->active, current_rule), NULL);
            }
            if (was_active) {
              Vertex *potential_vertex;
              for (k = 0; k < current_vertex->edges[j]->number_of_vertices;
                   ++k) {
                potential_vertex = current_vertex->edges[j]->from[k];
                if ((scene_literal_index(inference,
                                         potential_vertex->literal) >= 0) &&
                    (scene_literal_index(observation,
                                         potential_vertex->literal) == -1) &&
                    (potential_vertex !=
                     (Vertex *)_parent_vertex->front->data)) {
                  queue_push_back(_depths,
                                  (void *)((long)_depths->front->data) + 1);
                  queue_push_back(_vertices_to_check, (void *)potential_vertex);
                  queue_push_back(_parent_vertex,
                                  (void *)_vertices_to_check->front->data);
                }
              }
            }

            if (current_rule->weight <= 0) {
              vertex_remove_edge(current_vertex, j);
              --j;
            }
          }
        }
        queue_pop_front(_depths, NULL);
        queue_pop_front(_vertices_to_check, NULL);
        queue_pop_front(_parent_vertex, NULL);
      } while (_depths->size != 0);

      queue_destructor(&_depths);
      queue_destructor(&_vertices_to_check);
      queue_destructor(&_parent_vertex);
    }
  finished:
  }

  scene_destructor(&observed_and_inferred);
  scene_destructor(&opposing_literals);
}

#if (RULE_HYPERGRAPH_TEST_FUNCTIONS == 1) || (RULE_HYPERGRAPH_TEST == 1)

#include <check.h>

#undef ck_assert_rule_hypergraph_eq
#undef _ck_assert_rule_hypergraph_empty

/**
 * @brief Checks two Vertices to determine if they are equal or not. Do not use
 * the same pointer on both parameters.
 *
 * @param X The first Vertex to compare.
 * @param Y The second Vertex to compare.
 */
#define ck_assert_vertex_eq(X, Y)                                              \
  do {                                                                         \
    const Vertex *const _v1 = (X);                                             \
    const Vertex *const _v2 = (Y);                                             \
    ck_assert_ptr_nonnull(_v1);                                                \
    ck_assert_ptr_nonnull(_v2);                                                \
    ck_assert_ptr_ne(_v1, _v2);                                                \
    ck_assert_ptr_ne(_v1->literal, _v2->literal);                              \
    ck_assert_literal_eq(_v1->literal, _v2->literal);                          \
    ck_assert_int_eq(_v1->number_of_edges, _v2->number_of_edges);              \
    if (_v1->number_of_edges > 0) {                                            \
      ck_assert_ptr_ne(_v1->edges, _v2->edges);                                \
      unsigned int i;                                                          \
      for (i = 0; i < _v1->number_of_edges; ++i) {                             \
        ck_assert_ptr_ne(_v1->edges[i], _v2->edges[i]);                        \
      }                                                                        \
    }                                                                          \
  } while (0)

/**
 * @brief Checks two Edges to determine if they are equal or not. Do not use the
 * same pointer on both parameters.
 *
 * @param X The first Edge to compare.
 * @param Y The second Edge to compare.
 */
#define ck_assert_edge_eq(X, Y)                                                \
  do {                                                                         \
    const Edge *const _e1 = (X);                                               \
    const Edge *const _e2 = (Y);                                               \
    ck_assert_ptr_nonnull(_e1);                                                \
    ck_assert_ptr_nonnull(_e2);                                                \
    ck_assert_ptr_ne(_e1, _e2);                                                \
    ck_assert_ptr_ne(_e1->rule, _e2->rule);                                    \
    ck_assert_rule_eq(_e1->rule, _e2->rule);                                   \
    ck_assert_ptr_ne(_e1->rule->head, _e2->rule->head);                        \
    ck_assert_ptr_ne(_e1->rule->body, _e2->rule->body);                        \
    unsigned int i;                                                            \
    for (i = 0; i < _e1->rule->body->size; ++i) {                              \
      ck_assert_ptr_ne(_e1->rule->body->literals[i],                           \
                       _e2->rule->body->literals[i]);                          \
    }                                                                          \
    ck_assert_int_eq(_e1->number_of_vertices, _e2->number_of_vertices);        \
    ck_assert_ptr_ne(_e1->from, _e2->from);                                    \
    for (i = 0; i < _e1->number_of_vertices; ++i) {                            \
      ck_assert_ptr_ne(_e1->from[i], _e2->from[i]);                            \
      ck_assert_vertex_eq(_e1->from[i], _e2->from[i]);                         \
    }                                                                          \
  } while (0)

#define ck_assert_rule_hypergraph_eq(X, Y)                                     \
  do {                                                                         \
    const RuleHyperGraph *const _h1 = (X);                                     \
    const RuleHyperGraph *const _h2 = (Y);                                     \
    ck_assert_ptr_nonnull(_h1);                                                \
    ck_assert_ptr_nonnull(_h2);                                                \
    ck_assert_ptr_ne(_h1, _h2);                                                \
    ck_assert_ptr_nonnull(_h1->literal_tree);                                  \
    ck_assert_ptr_nonnull(_h2->literal_tree);                                  \
    ck_assert_ptr_ne(_h1->literal_tree, _h2->literal_tree);                    \
    struct prb_traverser _h1_traverser, _h2_traverser;                         \
    ck_assert_int_eq(_h1->literal_tree->prb_count,                             \
                     _h2->literal_tree->prb_count);                            \
    Vertex *_h1_current_vertex =                                               \
               (Vertex *)prb_t_first(&_h1_traverser, _h1->literal_tree),       \
           *_h2_current_vertex =                                               \
               (Vertex *)prb_t_first(&_h2_traverser, _h2->literal_tree);       \
    unsigned int i;                                                            \
    while (_h1_current_vertex) {                                               \
      ck_assert_vertex_eq(_h1_current_vertex, _h2_current_vertex);             \
      for (i = 0; i < _h1_current_vertex->number_of_edges; ++i) {              \
        ck_assert_edge_eq(_h1_current_vertex->edges[i],                        \
                          _h2_current_vertex->edges[i]);                       \
      }                                                                        \
      _h1_current_vertex = (Vertex *)prb_t_next(&_h1_traverser);               \
      _h2_current_vertex = (Vertex *)prb_t_next(&_h2_traverser);               \
    }                                                                          \
  } while (0)

#define _ck_assert_rule_hypergraph_empty(X, OP)                                \
  do {                                                                         \
    const RuleHyperGraph *const _h = (X);                                      \
    ck_assert_ptr_nonnull(_h);                                                 \
    ck_assert_ptr_nonnull(_h->literal_tree);                                   \
    _ck_assert_int(_h->literal_tree->prb_count, OP, 0);                        \
  } while (0)

#endif

#if RULE_HYPERGRAPH_TEST == 1

#include "../test/helper/literal.h"
#include "../test/helper/rule.h"
#include "../test/helper/rule_queue.h"

START_TEST(construct_destruct_hypergraph_test) {
  RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor(true);

  ck_assert_ptr_nonnull(hypergraph);
  ck_assert_int_eq(hypergraph->literal_tree->prb_count, 0);
  ck_assert_ptr_null(hypergraph->literal_tree->prb_param);
  ck_assert_ptr_null(hypergraph->literal_tree->prb_root);
  ck_assert_ptr_eq(hypergraph->literal_tree->prb_compare, compare_literals);
  ck_assert_rule_hypergraph_empty(hypergraph);

  rule_hypergraph_destructor(&hypergraph);
  ck_assert_ptr_null(hypergraph);

  rule_hypergraph_destructor(&hypergraph);
  ck_assert_ptr_null(hypergraph);
}
END_TEST

START_TEST(construct_destruct_vector_test) {
  Literal *l1 = literal_constructor("penguin", true),
          *l2 = literal_constructor("fly", false), *c1 = l1, *c2 = l2;

  Vertex *vertex1 = vertex_constructor(l1), *vertex2 = vertex_constructor(l2);
  ck_assert_ptr_nonnull(l1);
  ck_assert_ptr_nonnull(l2);

  ck_assert_ptr_eq(vertex1->literal, c1);
  ck_assert_ptr_null(vertex1->edges);
  ck_assert_int_eq(vertex1->number_of_edges, 0);

  ck_assert_ptr_eq(vertex2->literal, c2);
  ck_assert_ptr_null(vertex2->edges);
  ck_assert_int_eq(vertex2->number_of_edges, 0);

  ck_assert_ptr_nonnull(l1);
  vertex_destructor(&vertex1, true);
  ck_assert_ptr_null(vertex1);

  ck_assert_ptr_nonnull(l2);
  vertex_destructor(&vertex2, false);
  ck_assert_ptr_null(vertex2);
  ck_assert_ptr_nonnull(l2);

  literal_destructor(&l2);
}
END_TEST

START_TEST(construct_destruct_edge_test) {
  Literal *l1 = literal_constructor("penguin", true),
          *l2 = literal_constructor("fly", false),
          *l3 = literal_constructor("bird", true), *l_array[2] = {l1, l3}, *c1,
          *c2, *c3;
  literal_copy(&c1, l1);
  literal_copy(&c2, l2);
  literal_copy(&c3, l3);
  Vertex *v1 = vertex_constructor(l1), *v2 = vertex_constructor(l2),
         *v3 = vertex_constructor(l3), *v_array[2] = {v1, v3};
  Rule *r1 = rule_constructor(1, &l1, &l2, 0, false),
       *r2 = rule_constructor(2, l_array, &l2, 0, false),
       *r3 = rule_constructor(1, &c2, &c1, 0, false);
  RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor(true);

  prb_insert(hypergraph->literal_tree, v1);
  prb_insert(hypergraph->literal_tree, v2);
  prb_insert(hypergraph->literal_tree, v3);

  Edge *edge1 = edge_constructor(hypergraph, &r1, v2);
  ck_assert_ptr_nonnull(r1);
  ck_assert_ptr_eq(edge1->rule, r1);
  ck_assert_ptr_eq(edge1->rule->head, v2->literal);
  ck_assert_ptr_eq(edge1->rule->body->literals[0], v1->literal);
  ck_assert_ptr_nonnull(edge1->from);
  ck_assert_int_eq(edge1->number_of_vertices, 1);
  ck_assert_ptr_eq(edge1->from[0], v1);

  Edge *edge2 = edge_constructor(hypergraph, &r2, v2);
  ck_assert_ptr_nonnull(r2);
  ck_assert_ptr_eq(edge2->rule, r2);
  ck_assert_ptr_eq(edge2->rule->head, v2->literal);
  ck_assert_ptr_eq(edge2->rule->body->literals[0], v1->literal);
  ck_assert_ptr_eq(edge2->rule->body->literals[1], v3->literal);
  ck_assert_ptr_nonnull(edge2->from);
  ck_assert_int_eq(edge2->number_of_vertices, 2);
  unsigned int i;
  for (i = 0; i < edge2->number_of_vertices; ++i) {
    ck_assert_ptr_eq(edge2->from[i], v_array[i]);
  }

  ck_assert_ptr_ne(r3->head, v2->literal);
  ck_assert_ptr_ne(r3->body->literals[0], v1->literal);
  Edge *edge3 = edge_constructor(hypergraph, &r3, v1);
  ck_assert_ptr_nonnull(r3);
  ck_assert_ptr_eq(edge3->rule, r3);
  ck_assert_ptr_eq(edge3->rule->head, v1->literal);
  ck_assert_ptr_eq(edge3->rule->body->literals[0], v2->literal);
  ck_assert_ptr_nonnull(edge3->from);
  ck_assert_int_eq(edge3->number_of_vertices, 1);
  ck_assert_ptr_eq(edge3->from[0], v2);

  Rule *r4 = rule_constructor(1, &c3, &c2, 0.0, true), *r4_ptr = r4;
  ck_assert_ptr_ne(r4->head, v2->literal);
  ck_assert_ptr_ne(r4->body->literals[0], v3->literal);
  ck_assert_ptr_eq(r4->head, r4_ptr->head);
  ck_assert_ptr_eq(r4->body->literals[0], r4_ptr->body->literals[0]);
  Edge *edge4 = edge_constructor(hypergraph, &r4, v2);
  ck_assert_ptr_nonnull(r4);
  ck_assert_ptr_eq(edge4->rule, r4);
  ck_assert_ptr_ne(r4, r4_ptr);
  ck_assert_ptr_eq(edge4->rule->head, v2->literal);
  ck_assert_ptr_eq(edge4->rule->body->literals[0], v3->literal);
  ck_assert_ptr_nonnull(edge4->from);
  ck_assert_int_eq(edge4->number_of_vertices, 1);
  ck_assert_ptr_eq(edge4->from[0], v3);

  edge_destructor(&edge1);
  ck_assert_ptr_null(edge1);
  edge_destructor(&edge2);
  edge_destructor(&edge3);
  edge_destructor(&edge4);
  literal_destructor(&c1);

  rule_hypergraph_destructor(&hypergraph);
}
END_TEST

START_TEST(adding_edges_test) {
  RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor(true);
  Literal *l1 = literal_constructor("penguin", true),
          *l2 = literal_constructor("fly", false),
          *l3 = literal_constructor("bird", true), *l_array[2] = {l1, l3};
  Vertex *v2 = vertex_constructor(l2), *v3 = vertex_constructor(l3);
  Rule *r1 = rule_constructor(1, &l1, &l2, 0, false),
       *r2 = rule_constructor(1, &l1, &l3, 0, false),
       *r3 = rule_constructor(2, l_array, &l2, 0, false);

  prb_insert(hypergraph->literal_tree, v2);
  prb_insert(hypergraph->literal_tree, v3);

  Edge *e1 = edge_constructor(hypergraph, &r1, v2),
       *e2 = edge_constructor(hypergraph, &r2, v3);

  ck_assert_ptr_null(v2->edges);
  ck_assert_int_eq(v2->number_of_edges, 0);

  vertex_add_edge(v2, e1);
  ck_assert_ptr_nonnull(v2->edges);
  ck_assert_int_eq(v2->number_of_edges, 1);
  ck_assert_ptr_eq(v2->edges[0]->rule, r1);
  ck_assert_int_eq(v2->edges[0]->number_of_vertices, 1);
  ck_assert_literal_eq(v2->edges[0]->from[0]->literal, l1);

  ck_assert_ptr_null(v3->edges);
  ck_assert_int_eq(v3->number_of_edges, 0);

  vertex_add_edge(v3, e2);
  ck_assert_ptr_nonnull(v3->edges);
  ck_assert_int_eq(v3->number_of_edges, 1);
  ck_assert_ptr_eq(v3->edges[0]->rule, r2);
  ck_assert_int_eq(v3->edges[0]->number_of_vertices, 1);
  ck_assert_literal_eq(v3->edges[0]->from[0]->literal, l1);

  e1 = edge_constructor(hypergraph, &r3, v2);

  ck_assert_int_eq(v2->number_of_edges, 1);

  vertex_add_edge(v2, e1);
  ck_assert_int_eq(v2->number_of_edges, 2);
  ck_assert_ptr_eq(v2->edges[0]->rule, r1);
  ck_assert_int_eq(v2->edges[0]->number_of_vertices, 1);
  ck_assert_literal_eq(v2->edges[0]->from[0]->literal, l1);
  ck_assert_ptr_eq(v2->edges[1]->rule, r3);
  ck_assert_int_eq(v2->edges[1]->number_of_vertices, 2);
  ck_assert_literal_eq(v2->edges[1]->from[0]->literal, l1);
  ck_assert_literal_eq(v2->edges[1]->from[1]->literal, l3);

  rule_hypergraph_destructor(&hypergraph);
}
END_TEST

START_TEST(adding_rules_test) {
  RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor(true);
  Literal *l1 = literal_constructor("penguin", true),
          *l2 = literal_constructor("fly", false),
          *l3 = literal_constructor("bird", true),
          *l4 = literal_constructor("fly", true),
          *l5 = literal_constructor("wings", true), *l_array[2] = {l3, l2}, *c1,
          *c2;
  literal_copy(&c1, l1);
  literal_copy(&c2, l2);
  Vertex *v1 = vertex_constructor(l1), *v2 = vertex_constructor(l2),
         *v3 = vertex_constructor(l3), *v4 = vertex_constructor(l4),
         *v5 = vertex_constructor(l5), *current_v1, *current_v2;
  Rule *r1 = rule_constructor(1, &l1, &l2, 0, false),
       *r2 = rule_constructor(1, &l3, &l4, 0, false),
       *r3 = rule_constructor(1, &l3, &l1, 0, false),
       *r4 = rule_constructor(1, &l5, &l3, 0, false),
       *r5 = rule_constructor(2, l_array, &l1, 0, false);
  l_array[1] = l1;
  Rule *r6 = rule_constructor(2, l_array, &l2, 0, false);
  Edge *current_e;

  ck_assert_ptr_nonnull(r1);
  ck_assert_rule_hypergraph_empty(hypergraph);
  ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &r1), 1);
  ck_assert_rule_hypergraph_notempty(hypergraph);
  ck_assert_ptr_nonnull(r1);
  current_v1 = prb_find(hypergraph->literal_tree, v2);

  ck_assert_int_eq(current_v1->number_of_edges, 1);
  ck_assert_literal_eq(current_v1->literal, l2);
  ck_assert_ptr_nonnull(current_v1->edges);
  current_e = current_v1->edges[0];
  ck_assert_ptr_eq(current_e->rule, r1);
  ck_assert_ptr_nonnull(current_e->from);
  ck_assert_int_eq(current_e->number_of_vertices, 1);
  current_v2 = current_e->from[0];
  ck_assert_literal_eq(current_v2->literal, l1);
  ck_assert_ptr_null(current_v2->edges);
  ck_assert_int_eq(current_v2->number_of_edges, 0);

  ck_assert_ptr_nonnull(r3);
  ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &r3), 1);
  ck_assert_ptr_nonnull(r3);
  ck_assert_ptr_eq(current_v2->edges[0]->rule, r3);
  ck_assert_literal_eq(current_v2->literal, l1);
  ck_assert_ptr_nonnull(current_v2->edges);
  ck_assert_int_eq(current_v2->number_of_edges, 1);
  ck_assert_literal_eq(current_v1->literal, l2);
  ck_assert_literal_eq(current_v1->edges[0]->from[0]->literal, l1);
  ck_assert_literal_eq(
      current_v1->edges[0]->from[0]->edges[0]->from[0]->literal, l3);

  ck_assert_ptr_nonnull(r4);
  ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &r4), 1);
  ck_assert_ptr_nonnull(r4);
  current_v1 = prb_find(hypergraph->literal_tree, v3);
  ck_assert_literal_eq(current_v1->literal, l3);
  ck_assert_int_eq(current_v1->number_of_edges, 1);
  ck_assert_ptr_eq(current_v1->edges[0]->rule, r4);
  ck_assert_ptr_nonnull(current_v1->edges[0]->from);
  ck_assert_literal_eq(current_v1->edges[0]->from[0]->literal, l5);

  ck_assert_ptr_nonnull(r2);
  ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &r2), 1);
  ck_assert_ptr_nonnull(r2);
  current_v1 = prb_find(hypergraph->literal_tree, v4);
  ck_assert_literal_eq(current_v1->literal, l4);
  ck_assert_int_eq(current_v1->number_of_edges, 1);
  ck_assert_ptr_eq(current_v1->edges[0]->rule, r2);
  ck_assert_ptr_eq(current_v2->edges[0]->from[0],
                   current_v1->edges[0]->from[0]);

  ck_assert_ptr_nonnull(r5);
  ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &r5), 1);
  ck_assert_ptr_nonnull(r5);
  current_v1 = prb_find(hypergraph->literal_tree, v1);
  ck_assert_literal_eq(current_v1->literal, l1);
  ck_assert_int_eq(current_v1->number_of_edges, 2);

  current_e = current_v1->edges[0];
  ck_assert_ptr_eq(current_e->rule, r3);
  ck_assert_int_eq(current_e->number_of_vertices, 1);
  ck_assert_ptr_nonnull(current_e->from);
  ck_assert_literal_eq(current_e->from[0]->literal, l3);

  current_e = current_v1->edges[1];
  ck_assert_ptr_eq(current_e->rule, r5);
  ck_assert_int_eq(current_e->number_of_vertices, 2);
  ck_assert_ptr_nonnull(current_e->from);

  ck_assert_literal_eq(current_e->from[0]->literal, l3);
  ck_assert_ptr_eq(current_e->from[0]->edges[0]->rule, r4);
  ck_assert_int_eq(current_e->from[0]->number_of_edges, 1);
  ck_assert_literal_eq(current_e->from[0]->edges[0]->from[0]->literal, l5);
  ck_assert_literal_eq(current_e->from[1]->literal, l2);
  ck_assert_ptr_eq(current_e->from[1]->edges[0]->rule, r1);
  ck_assert_int_eq(current_e->from[1]->number_of_edges, 1);
  ck_assert_literal_eq(current_e->from[1]->edges[0]->from[0]->literal, l1);

  ck_assert_ptr_nonnull(r6);
  ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &r6), 1);
  ck_assert_ptr_nonnull(r6);
  current_v1 = prb_find(hypergraph->literal_tree, v2);
  ck_assert_literal_eq(current_v1->literal, l2);
  ck_assert_int_eq(current_v1->number_of_edges, 2);

  current_e = current_v1->edges[0];
  ck_assert_ptr_eq(current_e->rule, r1);
  ck_assert_int_eq(current_e->number_of_vertices, 1);
  ck_assert_ptr_nonnull(current_e->from);
  ck_assert_literal_eq(current_e->from[0]->literal, l1);
  ck_assert_int_eq(current_e->from[0]->number_of_edges, 2);
  ck_assert_ptr_eq(current_e->from[0]->edges[0]->rule, r3);
  ck_assert_ptr_eq(current_e->from[0]->edges[1]->rule, r5);

  current_e = current_v1->edges[1];
  ck_assert_ptr_eq(current_e->rule, r6);
  ck_assert_int_eq(current_e->number_of_vertices, 2);
  ck_assert_ptr_nonnull(current_e->from);
  ck_assert_literal_eq(current_e->from[0]->literal, l3);
  ck_assert_int_eq(current_e->from[0]->number_of_edges, 1);
  ck_assert_ptr_eq(current_e->from[0]->edges[0]->rule, r4);
  ck_assert_literal_eq(current_e->from[1]->literal, l1);
  ck_assert_int_eq(current_e->from[1]->number_of_edges, 2);
  ck_assert_ptr_eq(current_e->from[1]->edges[0]->rule, r3);
  ck_assert_ptr_eq(current_e->from[1]->edges[1]->rule, r5);

  ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &r6), 0);

  Rule *r7 = rule_constructor(1, &c2, &c1, 0, true), *r7_ptr = r7, *copy;
  rule_copy(&copy, r7);
  ck_assert_ptr_nonnull(r7);
  ck_assert_ptr_eq(r7, r7_ptr);
  rule_hypergraph_add_rule(hypergraph, &r7);
  ck_assert_ptr_nonnull(r7);
  ck_assert_ptr_ne(r7, r7_ptr);
  current_v1 = prb_find(hypergraph->literal_tree, v1);
  ck_assert_int_eq(current_v1->number_of_edges, 3);
  ck_assert_ptr_eq(current_v1->edges[2]->rule, r7);
  ck_assert_ptr_ne(current_v1->edges[2]->rule, r7_ptr);
  ck_assert_rule_eq(current_v1->edges[2]->rule, copy);
  ck_assert_int_eq(current_v1->edges[2]->number_of_vertices, 1);
  ck_assert_literal_eq(current_v1->edges[2]->from[0]->literal, l2);
  rule_destructor(&copy);

  Literal *l6 = literal_constructor("chicken", true),
          *l7 = literal_constructor("feathers", true), *c6, *c7;
  Vertex *v6 = vertex_constructor(l6);
  literal_copy(&c2, l2);
  literal_copy(&c6, l6);
  literal_copy(&c7, l7);
  l_array[0] = c2;
  l_array[1] = c7;
  Rule *r8 = rule_constructor(2, l_array, &c6, 0, true), *r8_ptr = r8;
  rule_copy(&copy, r8);
  ck_assert_ptr_nonnull(r8);
  ck_assert_ptr_eq(r8, r8_ptr);
  rule_hypergraph_add_rule(hypergraph, &r8);
  ck_assert_ptr_nonnull(r8);
  ck_assert_ptr_ne(r8, r8_ptr);
  current_v2 = prb_find(hypergraph->literal_tree, v6);
  ck_assert_int_eq(current_v2->number_of_edges, 1);
  ck_assert_ptr_eq(current_v2->edges[0]->rule, r8);
  ck_assert_ptr_ne(current_v2->edges[0]->rule, r8_ptr);
  ck_assert_rule_eq(current_v2->edges[0]->rule, copy);
  ck_assert_int_eq(current_v2->edges[0]->number_of_vertices, 2);
  ck_assert_literal_eq(current_v2->edges[0]->from[0]->literal, l2);
  ck_assert_literal_eq(current_v2->edges[0]->from[1]->literal, l7);

  ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &copy), 0);

  ck_assert_int_eq(rule_hypergraph_add_rule(NULL, &copy), -1);
  rule_destructor(&copy);

  ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &copy), -1);

  vertex_destructor(&v1, false);
  vertex_destructor(&v2, false);
  vertex_destructor(&v3, false);
  vertex_destructor(&v4, false);
  vertex_destructor(&v5, false);
  vertex_destructor(&v6, false);
  literal_destructor(&l6);
  literal_destructor(&l7);
  rule_hypergraph_destructor(&hypergraph);
}
END_TEST

START_TEST(removing_rules_test) {
  RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor(true);
  Literal *l1 = literal_constructor("penguin", true),
          *l2 = literal_constructor("fly", false),
          *l3 = literal_constructor("bird", true),
          *l4 = literal_constructor("fly", true),
          *l5 = literal_constructor("wings", true), *l_array[2] = {l3, l5};
  Vertex *v1 = vertex_constructor(l1), *v2 = vertex_constructor(l2),
         *v3 = vertex_constructor(l3), *v4 = vertex_constructor(l4),
         *v5 = vertex_constructor(l5), *current_v1, *current_v2;
  Rule *r1 = rule_constructor(1, &l1, &l2, 0, false),
       *r2 = rule_constructor(1, &l3, &l4, 0, false),
       *r3 = rule_constructor(1, &l5, &l4, 0, false),
       *r4 = rule_constructor(1, &l5, &l3, 0, false),
       *r5 = rule_constructor(2, l_array, &l4, 0, false);
  l_array[1] = l1;
  Rule *r6 = rule_constructor(2, l_array, &l2, 0, false), *copy;

  rule_hypergraph_add_rule(hypergraph, &r1);
  rule_hypergraph_add_rule(hypergraph, &r2);
  rule_hypergraph_add_rule(hypergraph, &r3);
  rule_hypergraph_add_rule(hypergraph, &r4);
  rule_hypergraph_add_rule(hypergraph, &r5);
  rule_hypergraph_add_rule(hypergraph, &r6);

  current_v1 = prb_find(hypergraph->literal_tree, v3);
  ck_assert_literal_eq(current_v1->literal, l3);
  ck_assert_int_eq(current_v1->number_of_edges, 1);
  ck_assert_ptr_eq(current_v1->edges[0]->rule, r4);

  rule_copy(&copy, r4);
  rule_hypergraph_remove_rule(hypergraph, r4);
  ck_assert_literal_eq(current_v1->literal, l3);
  ck_assert_int_eq(current_v1->number_of_edges, 0);
  ck_assert_ptr_null(current_v1->edges);

  rule_hypergraph_remove_rule(hypergraph, copy);
  rule_destructor(&copy);

  current_v1 = prb_find(hypergraph->literal_tree, v4);
  ck_assert_int_eq(current_v1->number_of_edges, 3);
  ck_assert_ptr_eq(current_v1->edges[0]->rule, r2);
  ck_assert_ptr_eq(current_v1->edges[1]->rule, r3);
  ck_assert_ptr_eq(current_v1->edges[2]->rule, r5);

  rule_hypergraph_remove_rule(hypergraph, r3);
  ck_assert_int_eq(current_v1->number_of_edges, 2);
  ck_assert_ptr_eq(current_v1->edges[0]->rule, r2);
  ck_assert_ptr_eq(current_v1->edges[1]->rule, r5);

  rule_hypergraph_remove_rule(hypergraph, r2);
  ck_assert_int_eq(current_v1->number_of_edges, 1);
  ck_assert_ptr_eq(current_v1->edges[0]->rule, r5);

  current_v2 = prb_find(hypergraph->literal_tree, v2);
  ck_assert_int_eq(current_v2->number_of_edges, 2);
  ck_assert_ptr_eq(current_v2->edges[0]->rule, r1);
  ck_assert_ptr_eq(current_v2->edges[1]->rule, r6);

  rule_hypergraph_remove_rule(hypergraph, r1);
  ck_assert_int_eq(current_v2->number_of_edges, 1);
  ck_assert_ptr_eq(current_v2->edges[0]->rule, r6);
  ck_assert_int_eq(current_v1->number_of_edges, 1);
  ck_assert_ptr_eq(current_v1->edges[0]->rule, r5);

  rule_hypergraph_remove_rule(hypergraph, r5);
  ck_assert_int_eq(current_v1->number_of_edges, 0);
  ck_assert_ptr_null(current_v1->edges);
  ck_assert_int_eq(current_v2->number_of_edges, 1);
  ck_assert_ptr_eq(current_v2->edges[0]->rule, r6);

  ck_assert_rule_hypergraph_notempty(hypergraph);
  rule_hypergraph_remove_rule(hypergraph, r6);
  ck_assert_int_eq(current_v2->number_of_edges, 0);
  ck_assert_ptr_null(current_v2->edges);
  ck_assert_int_eq(current_v1->number_of_edges, 0);
  ck_assert_ptr_null(current_v1->edges);
  ck_assert_rule_hypergraph_notempty(hypergraph);

  vertex_destructor(&v1, false);
  vertex_destructor(&v2, false);
  vertex_destructor(&v3, false);
  vertex_destructor(&v4, false);
  vertex_destructor(&v5, false);
  rule_hypergraph_destructor(&hypergraph);
}
END_TEST

START_TEST(get_inactive_rules_test) {
  KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0, true);
  Literal *l1 = literal_constructor("penguin", true),
          *l2 = literal_constructor("fly", false),
          *l3 = literal_constructor("bird", true),
          *l4 = literal_constructor("fly", true),
          *l5 = literal_constructor("wings", true), *l_array[2] = {l3, l5};
  Rule *r1 = rule_constructor(1, &l1, &l2, 5, false),
       *r2 = rule_constructor(1, &l3, &l4, 3, false),
       *r3 = rule_constructor(1, &l5, &l4, 2.9, false),
       *r4 = rule_constructor(1, &l5, &l3, 3.1, false),
       *r5 = rule_constructor(2, l_array, &l4, 0.01, false);
  l_array[1] = l1;
  Rule *r6 = rule_constructor(2, l_array, &l2, 4, false);

  rule_hypergraph_add_rule(knowledge_base->hypergraph, &r1);
  rule_hypergraph_add_rule(knowledge_base->hypergraph, &r2);
  rule_hypergraph_add_rule(knowledge_base->hypergraph, &r3);
  rule_hypergraph_add_rule(knowledge_base->hypergraph, &r4);

  RuleQueue *result = NULL;
  ck_assert_ptr_null(result);
  rule_hypergraph_get_inactive_rules(knowledge_base, &result);
  ck_assert_ptr_nonnull(result);
  ck_assert_int_eq(result->length, 1);
  ck_assert_rule_eq(result->rules[0], r3);
  rule_queue_destructor(&result);

  rule_hypergraph_add_rule(knowledge_base->hypergraph, &r5);
  rule_hypergraph_get_inactive_rules(knowledge_base, &result);
  ck_assert_int_eq(result->length, 2);
  ck_assert_rule_eq(result->rules[0], r3);
  ck_assert_rule_eq(result->rules[1], r5);
  rule_queue_destructor(&result);

  rule_hypergraph_add_rule(knowledge_base->hypergraph, &r6);
  rule_hypergraph_get_inactive_rules(knowledge_base, &result);
  ck_assert_int_eq(result->length, 2);
  ck_assert_rule_eq(result->rules[0], r3);
  ck_assert_rule_eq(result->rules[1], r5);
  rule_queue_destructor(&result);

  rule_hypergraph_remove_rule(knowledge_base->hypergraph, r3);
  rule_hypergraph_get_inactive_rules(knowledge_base, &result);
  ck_assert_int_eq(result->length, 1);
  ck_assert_rule_eq(result->rules[0], r5);
  rule_queue_destructor(&result);

  rule_hypergraph_remove_rule(knowledge_base->hypergraph, r2);
  rule_hypergraph_get_inactive_rules(knowledge_base, &result);
  ck_assert_int_eq(result->length, 1);
  ck_assert_rule_eq(result->rules[0], r5);
  rule_queue_destructor(&result);

  knowledge_base_destructor(&knowledge_base);
}
END_TEST

START_TEST(hypergraph_copy_test) {
  KnowledgeBase *kb1 = knowledge_base_constructor(10, true), *kb2 = NULL;
  Literal *l1 = literal_constructor("penguin", true),
          *l2 = literal_constructor("fly", false),
          *l3 = literal_constructor("bird", true),
          *l4 = literal_constructor("fly", true),
          *l5 = literal_constructor("wings", true), *l_array[2] = {l3, l5};
  Rule *r1 = rule_constructor(1, &l1, &l2, 0, false),
       *r2 = rule_constructor(1, &l3, &l4, 0, false),
       *r3 = rule_constructor(1, &l5, &l4, 0, false),
       *r4 = rule_constructor(1, &l5, &l3, 0, false),
       *r5 = rule_constructor(2, l_array, &l4, 0, false);
  l_array[1] = l1;
  Rule *r6 = rule_constructor(2, l_array, &l2, 0, false);

  rule_hypergraph_add_rule(kb1->hypergraph, &r1);
  rule_hypergraph_add_rule(kb1->hypergraph, &r2);
  rule_hypergraph_add_rule(kb1->hypergraph, &r3);
  rule_hypergraph_add_rule(kb1->hypergraph, &r4);
  rule_hypergraph_add_rule(kb1->hypergraph, &r5);
  rule_hypergraph_add_rule(kb1->hypergraph, &r6);

  ck_assert_ptr_null(kb2);
  kb2 = (KnowledgeBase *)malloc(sizeof(KnowledgeBase));
  kb2->activation_threshold = kb1->activation_threshold;
  kb2->active = rule_queue_constructor(false);
  rule_hypergraph_copy(&kb2, kb1);
  ck_assert_ptr_nonnull(kb2->hypergraph);
  ck_assert_ptr_ne(kb1->hypergraph, kb2->hypergraph);
  ck_assert_ptr_ne(kb1->hypergraph->literal_tree,
                   kb2->hypergraph->literal_tree);

  struct prb_traverser h1_traverser, h2_traverser;

  Vertex *h1_vertex = prb_t_first(&h1_traverser, kb1->hypergraph->literal_tree),
         *h2_vertex = prb_t_first(&h2_traverser, kb2->hypergraph->literal_tree);

  unsigned int i;
  size_t total_vertices = 0, total_edges = 0;
  while (h1_vertex && h2_vertex) {
    ++total_vertices;
    ck_assert_vertex_eq(h1_vertex, h2_vertex);

    total_edges += h1_vertex->number_of_edges;
    for (i = 0; i < h1_vertex->number_of_edges; ++i) {
      ck_assert_edge_eq(h1_vertex->edges[i], h2_vertex->edges[i]);
    }

    h1_vertex = prb_t_next(&h1_traverser);
    h2_vertex = prb_t_next(&h2_traverser);
  }
  ck_assert_rule_hypergraph_eq(kb1->hypergraph, kb2->hypergraph);

  knowledge_base_destructor(&kb1);
  ck_assert_ptr_null(kb1);
  ck_assert_ptr_nonnull(kb2);

  h2_vertex = prb_t_first(&h2_traverser, kb2->hypergraph->literal_tree);
  size_t copy_total_vertices = 0, copy_total_edges = 0;
  while (h2_vertex) {
    ++copy_total_vertices;

    copy_total_edges += h2_vertex->number_of_edges;

    h2_vertex = prb_t_next(&h2_traverser);
  }

  ck_assert_int_eq(total_vertices, copy_total_vertices);
  ck_assert_int_eq(total_edges, copy_total_edges);

  rule_hypergraph_copy(NULL, kb2);
  ck_assert_ptr_nonnull(kb2->hypergraph);

  knowledge_base_copy(&kb1, NULL);
  ck_assert_ptr_null(kb1);

  knowledge_base_destructor(&kb2);
  ck_assert_ptr_null(kb2);
}
END_TEST

void reset_active_rules_weight(KnowledgeBase *knowledge_base,
                               float new_weight) {
  unsigned int i;
  for (i = 0; i < knowledge_base->active->length; ++i) {
    knowledge_base->active->rules[i]->weight = new_weight;
  }
}

START_TEST(update_rules_test) {
  KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0, false);
  Literal *penguin = literal_constructor("penguin", true),
          *fly = literal_constructor("fly", true),
          *bird = literal_constructor("bird", true),
          *n_fly = literal_constructor("fly", false),
          *wings = literal_constructor("wings", true),
          *l_array[2] = {bird, wings};
  Rule *penguin_fly = rule_constructor(1, &penguin, &fly, 0, false),
       *bird_n_fly = rule_constructor(1, &bird, &n_fly, 0, false),
       *wings_n_fly = rule_constructor(1, &wings, &n_fly, 0, false),
       *wings_bird = rule_constructor(1, &wings, &bird, 0, false),
       *bird_wings_n_fly = rule_constructor(2, l_array, &n_fly, 0, false);
  l_array[1] = penguin;
  Rule *bird_penguin_fly = rule_constructor(2, l_array, &fly, 0, false);

  rule_hypergraph_add_rule(knowledge_base->hypergraph, &penguin_fly);
  rule_hypergraph_add_rule(knowledge_base->hypergraph, &bird_n_fly);
  rule_hypergraph_add_rule(knowledge_base->hypergraph, &wings_n_fly);
  rule_hypergraph_add_rule(knowledge_base->hypergraph, &wings_bird);
  rule_hypergraph_add_rule(knowledge_base->hypergraph, &bird_wings_n_fly);
  rule_hypergraph_add_rule(knowledge_base->hypergraph, &bird_penguin_fly);

  Scene *observation = scene_constructor(false),
        *inference = scene_constructor(false);

  ck_assert_float_eq_tol(penguin_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_penguin_fly->weight, 0, 0.000001);
  ck_assert_rule_queue_empty(knowledge_base->active);

  RuleQueue *inactive_rules;

  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 6);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(penguin_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_penguin_fly->weight, 0, 0.000001);
  ck_assert_rule_queue_empty(knowledge_base->active);

  scene_add_literal(observation, &penguin);
  scene_add_literal(observation, &wings);
  ck_assert_float_eq_tol(penguin_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_penguin_fly->weight, 0, 0.000001);
  ck_assert_rule_queue_empty(knowledge_base->active);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 6);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(penguin_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_penguin_fly->weight, 0, 0.000001);
  ck_assert_rule_queue_empty(knowledge_base->active);

  scene_add_literal(observation, &fly);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 5);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(penguin_fly->weight, 1.5, 0.000001);
  ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
  ck_assert_float_eq_tol(bird_penguin_fly->weight, 0, 0.000001);
  ck_assert_rule_queue_empty(knowledge_base->active);

  scene_add_literal(observation, &bird);
  ck_assert_rule_queue_empty(knowledge_base->active);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 2);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(penguin_fly->weight, 3.0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 1.5, 0.000001);
  ck_assert_float_eq_tol(bird_penguin_fly->weight, 1.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 1);
  ck_assert_rule_eq(knowledge_base->active->rules[0], penguin_fly);

  scene_remove_literal(observation, observation->size - 1, NULL);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 2);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(penguin_fly->weight, 4.5, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 1.5, 0.000001);
  ck_assert_float_eq_tol(bird_penguin_fly->weight, 1.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 1);
  ck_assert_rule_eq(knowledge_base->active->rules[0], penguin_fly);

  wings_n_fly = rule_constructor(1, &wings, &n_fly, 0, false);
  knowledge_base_add_rule(knowledge_base, &wings_n_fly);

  scene_remove_literal(observation, observation->size - 1, NULL);
  scene_add_literal(observation, &n_fly);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 4);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(penguin_fly->weight, 2.5, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 1.5, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 1.5, 0.000001);
  ck_assert_float_eq_tol(bird_penguin_fly->weight, 1.5, 0.000001);
  ck_assert_rule_queue_empty(knowledge_base->active);

  bird_n_fly = rule_constructor(1, &bird, &n_fly, 0, false);
  knowledge_base_add_rule(knowledge_base, &bird_n_fly);
  l_array[0] = bird;
  l_array[1] = wings;
  bird_wings_n_fly = rule_constructor(2, l_array, &n_fly, 0, false);
  knowledge_base_add_rule(knowledge_base, &bird_wings_n_fly);

  scene_remove_literal(observation, 1, NULL);
  scene_add_literal(observation, &bird);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 2.25, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 5);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(penguin_fly->weight, 0.5, 0.000001);
  ck_assert_float_eq_tol(bird_n_fly->weight, 2.25, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 1.5, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 1.5, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
  ck_assert_rule_queue_empty(knowledge_base->active);

  scene_add_literal(observation, &wings);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 1);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(bird_n_fly->weight, 3.75, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 3.0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 3.0, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 3);
  ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], bird_n_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[2], wings_bird);

  scene_destructor(&observation);
  observation = scene_constructor(false);
  scene_destructor(&inference);
  inference = scene_constructor(false);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(bird_n_fly->weight, 3.75, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 3.0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 3.0, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 3);
  ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], bird_n_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[2], wings_bird);

  scene_destructor(&inference);
  scene_add_literal(observation, &bird);
  scene_add_literal(observation, &fly);
  scene_copy(&inference, observation);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 1);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(bird_n_fly->weight, 3.75, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 3.0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 3.0, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 3);
  ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], bird_n_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[2], wings_bird);

  scene_destructor(&inference);
  scene_add_literal(observation, &bird);
  scene_add_literal(observation, &fly);
  inference = scene_constructor(false);
  scene_add_literal(inference, &bird);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 2);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(bird_n_fly->weight, 1.75, 0.000001);
  ck_assert_float_eq_tol(wings_n_fly->weight, 3.0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 3.0, 0.000001);
  ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 2);
  ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], wings_bird);

  scene_add_literal(inference, &wings);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 1);
  rule_queue_destructor(&inactive_rules);
  ck_assert_float_eq_tol(wings_n_fly->weight, 1.0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 3.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 1);
  ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);

  scene_destructor(&observation);
  scene_destructor(&inference);
  observation = scene_constructor(false);
  inference = scene_constructor(false);
  scene_add_literal(observation, &bird);
  scene_add_literal(inference, &n_fly);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(wings_n_fly->weight, 1.0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 3.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 1);
  ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);

  scene_remove_literal(observation, 0, NULL);
  scene_add_literal(observation, &wings);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(wings_n_fly->weight, 1.0, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 3.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 1);
  ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);

  scene_remove_literal(observation, 0, NULL);
  scene_remove_literal(inference, 0, NULL);
  scene_add_literal(observation, &fly);
  scene_add_literal(inference, &wings);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(wings_bird->weight, 3.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 1);
  ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);

  Rule *wings_fly = rule_constructor(1, &wings, &fly, 2.0, false);
  knowledge_base_add_rule(knowledge_base, &wings_fly);

  scene_remove_literal(observation, 0, NULL);
  scene_remove_literal(inference, 0, NULL);
  scene_add_literal(observation, &wings);
  scene_add_literal(observation, &fly);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(wings_bird->weight, 3.5, 0.000001);
  ck_assert_float_eq_tol(wings_fly->weight, 2.5, 0.000001);
  ck_assert_rule_queue_notempty(knowledge_base->active);
  ck_assert_int_eq(knowledge_base->active->length, 1);
  ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);

  scene_destructor(&observation);
  scene_destructor(&inference);

  // Back-ward chaining cases

  knowledge_base_destructor(&knowledge_base);
  knowledge_base = knowledge_base_constructor(3.0, true);
  bird = literal_constructor("bird", true);
  fly = literal_constructor("fly", true);
  penguin = literal_constructor("penguin", true);
  n_fly = literal_constructor("fly", false);

  Rule *bird_fly = rule_constructor(1, &bird, &fly, 5.0, false),
       *penguin_bird = rule_constructor(1, &penguin, &bird, 5.0, false);

  knowledge_base_add_rule(knowledge_base, &bird_fly);
  knowledge_base_add_rule(knowledge_base, &penguin_bird);

  observation = scene_constructor(false);
  scene_add_literal(observation, &penguin);
  scene_add_literal(observation, &n_fly);
  inference = scene_constructor(false);
  scene_add_literal(inference, &bird);
  scene_add_literal(inference, &fly);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(bird_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_bird->weight, 4.5, 0.000001);

  reset_active_rules_weight(knowledge_base, 5);
  Literal *antarctica = literal_constructor("antarctica", true),
          *feathers = literal_constructor("feathers", true);
  wings = literal_constructor("wings", true);

  l_array[0] = antarctica;
  l_array[1] = bird;
  Rule *feathers_bird = rule_constructor(1, &feathers, &bird, 5, false),
       *penguin_wings = rule_constructor(1, &penguin, &wings, 5, false),
       *antarctica_bird_fly = rule_constructor(2, l_array, &fly, 5, false),
       *penguin_antarctica =
           rule_constructor(1, &penguin, &antarctica, 5, false);
  wings_bird = rule_constructor(1, &wings, &bird, 5, false);

  knowledge_base_add_rule(knowledge_base, &feathers_bird);
  knowledge_base_add_rule(knowledge_base, &wings_bird);
  knowledge_base_add_rule(knowledge_base, &penguin_wings);
  knowledge_base_add_rule(knowledge_base, &antarctica_bird_fly);
  knowledge_base_add_rule(knowledge_base, &penguin_antarctica);
  scene_add_literal(inference, &wings);
  scene_add_literal(inference, &antarctica);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(bird_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_bird->weight, 4, 0.000001);
  ck_assert_float_eq_tol(feathers_bird->weight, 5, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_wings->weight, 4.333333, 0.000001);
  ck_assert_float_eq_tol(antarctica_bird_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_antarctica->weight, 4.5, 0.000001);

  reset_active_rules_weight(knowledge_base, 5);
  scene_add_literal(observation, &antarctica);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(bird_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_bird->weight, 4, 0.000001);
  ck_assert_float_eq_tol(feathers_bird->weight, 5, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_wings->weight, 4.333333, 0.000001);
  ck_assert_float_eq_tol(antarctica_bird_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_antarctica->weight, 5.5, 0.000001);

  reset_active_rules_weight(knowledge_base, 5);
  Rule *antarctica_penguin =
      rule_constructor(1, &antarctica, &penguin, 5, false);
  knowledge_base_add_rule(knowledge_base, &antarctica_penguin);
  scene_remove_literal(observation,
                       scene_literal_index(observation, antarctica), NULL);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(bird_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_bird->weight, 4, 0.000001);
  ck_assert_float_eq_tol(feathers_bird->weight, 5, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_wings->weight, 4.333333, 0.000001);
  ck_assert_float_eq_tol(antarctica_bird_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(antarctica_penguin->weight, 5.5, 0.000001);
  ck_assert_float_eq_tol(penguin_antarctica->weight, 4.5, 0.000001);

  wings_fly = rule_constructor(1, &wings, &fly, 2.5, false);
  Rule *wings_feathers = rule_constructor(1, &wings, &feathers, 2.5, false);
  size_t initial_total_active_rules = knowledge_base->active->length;
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  size_t initial_total_inactive_rules = inactive_rules->length;
  rule_queue_destructor(&inactive_rules);

  reset_active_rules_weight(knowledge_base, 5);
  knowledge_base_add_rule(knowledge_base, &wings_fly);
  knowledge_base_add_rule(knowledge_base, &wings_feathers);
  ck_assert_int_eq(knowledge_base->active->length, initial_total_active_rules);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_gt(inactive_rules->length, initial_total_inactive_rules);
  initial_total_inactive_rules = inactive_rules->length;
  rule_queue_destructor(&inactive_rules);
  scene_add_literal(observation, &feathers);

  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1,
                               false, NULL, 0, NULL);
  ck_assert_float_eq_tol(bird_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_bird->weight, 4, 0.000001);
  ck_assert_float_eq_tol(feathers_bird->weight, 4, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_wings->weight, 4.333333, 0.000001);
  ck_assert_float_eq_tol(antarctica_bird_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(antarctica_penguin->weight, 5.5, 0.000001);
  ck_assert_float_eq_tol(penguin_antarctica->weight, 4.5, 0.000001);
  ck_assert_float_eq_tol(wings_fly->weight, 1.5, 0.000001);
  ck_assert_float_eq_tol(wings_feathers->weight, 3, 0.000001);

  ck_assert_int_gt(knowledge_base->active->length, initial_total_active_rules);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_lt(inactive_rules->length, initial_total_inactive_rules);
  rule_queue_destructor(&inactive_rules);

  literal_destructor(&n_fly);

  Literal *eagle = literal_constructor("eagle", true);
  Rule *fly_eagle = rule_constructor(1, &fly, &eagle, 2, false);
  knowledge_base_add_rule(knowledge_base, &fly_eagle);

  reset_active_rules_weight(knowledge_base, 5);
  scene_destructor(&observation);
  scene_destructor(&inference);
  observation = scene_constructor(false);
  inference = scene_constructor(false);

  scene_add_literal(observation, &penguin);
  scene_add_literal(inference, &wings);
  scene_add_literal(inference, &bird);
  scene_add_literal(inference, &feathers);
  scene_add_literal(inference, &fly);
  scene_add_literal(inference, &eagle);
  char *header[] = {""};
  Scene *birds = scene_constructor(false);
  scene_add_literal(birds, &penguin);
  scene_add_literal(birds, &eagle);
  Scene *incompatibility[] = {birds};
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1,
                               false, header, 1, incompatibility);
  ck_assert_float_eq_tol(bird_fly->weight, 5, 0.000001);
  ck_assert_float_eq_tol(penguin_bird->weight, 5, 0.000001);
  ck_assert_float_eq_tol(feathers_bird->weight, 5, 0.000001);
  ck_assert_float_eq_tol(wings_bird->weight, 5, 0.000001);
  ck_assert_float_eq_tol(penguin_wings->weight, 5, 0.000001);
  ck_assert_float_eq_tol(antarctica_bird_fly->weight, 5, 0.000001);
  ck_assert_float_eq_tol(antarctica_penguin->weight, 5, 0.000001);
  ck_assert_float_eq_tol(penguin_antarctica->weight, 5, 0.000001);
  ck_assert_float_eq_tol(wings_fly->weight, 1.5, 0.000001);
  ck_assert_float_eq_tol(wings_feathers->weight, 5, 0.000001);
  ck_assert_float_eq_tol(fly_eagle->weight, 1, 0.000001);

  scene_destructor(&birds);
  scene_destructor(&observation);
  scene_destructor(&inference);
  knowledge_base_destructor(&knowledge_base);

  knowledge_base = knowledge_base_constructor(3.0, false);

  penguin = literal_constructor_from_string("bird_penguin"),
  fly = literal_constructor_from_string("fly_yes"),
  n_fly = literal_constructor_from_string("fly_no");
  eagle = literal_constructor_from_string("bird_eagle");
  Literal *n_wings = literal_constructor_from_string("wings_no");
  wings = literal_constructor_from_string("wings_yes");

  const size_t header_size = 3;
  char *headers[] = {"bird", "fly", "wings"};
  birds = scene_constructor(false);
  Scene *fly_labels = scene_constructor(false),
        *wings_labels = scene_constructor(false);
  scene_add_literal(birds, &penguin);
  scene_add_literal(birds, &eagle);
  scene_add_literal(fly_labels, &fly);
  scene_add_literal(fly_labels, &n_fly);
  scene_add_literal(wings_labels, &wings);
  scene_add_literal(wings_labels, &n_wings);
  Scene *incompatibilities[] = {birds, fly_labels, wings_labels};

  Rule *eagle_fly = rule_constructor(1, &eagle, &fly, 3, false),
       *eagle_wings = rule_constructor(1, &eagle, &wings, 3, false);
  penguin_fly = rule_constructor(1, &penguin, &fly, 4, false);

  knowledge_base_add_rule(knowledge_base, &penguin_fly);
  knowledge_base_add_rule(knowledge_base, &eagle_fly);
  knowledge_base_add_rule(knowledge_base, &eagle_wings);

  observation = scene_constructor(false);
  scene_add_literal(observation, &penguin);
  scene_add_literal(observation, &n_fly);
  inference = scene_constructor(false);
  ck_assert_int_eq(knowledge_base->active->length, 3);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 0);
  rule_queue_destructor(&inactive_rules);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2,
                               false, NULL, 0, NULL);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 0);
  rule_queue_destructor(&inactive_rules);
  ck_assert_int_eq(knowledge_base->active->length, 3);
  ck_assert_rule_eq(knowledge_base->active->rules[0], penguin_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[2], eagle_wings);

  ck_assert_int_eq(knowledge_base->active->length, 3);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 0);
  rule_queue_destructor(&inactive_rules);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2,
                               false, headers, header_size, incompatibilities);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 1);
  rule_queue_destructor(&inactive_rules);
  ck_assert_int_eq(knowledge_base->active->length, 2);
  ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);

  scene_remove_literal(observation, scene_literal_index(observation, n_fly),
                       NULL);
  ck_assert_int_eq(knowledge_base->active->length, 2);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 1);
  rule_queue_destructor(&inactive_rules);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2,
                               false, headers, header_size, incompatibilities);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 1);
  rule_queue_destructor(&inactive_rules);
  ck_assert_int_eq(knowledge_base->active->length, 2);
  ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);

  scene_add_literal(inference, &n_fly);
  ck_assert_int_eq(knowledge_base->active->length, 2);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 1);
  rule_queue_destructor(&inactive_rules);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2,
                               false, headers, header_size, incompatibilities);
  rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
  ck_assert_int_eq(inactive_rules->length, 1);
  rule_queue_destructor(&inactive_rules);
  ck_assert_int_eq(knowledge_base->active->length, 2);
  ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);

  wings_fly = rule_constructor(1, &wings, &fly, 2, false);
  Rule *penguin_n_fly = rule_constructor(1, &penguin, &n_fly, 3, false),
       *wings_eagle = rule_constructor(1, &wings, &eagle, 2, false);

  knowledge_base_add_rule(knowledge_base, &wings_eagle);

  scene_destructor(&observation);
  observation = scene_constructor(false);
  scene_add_literal(observation, &eagle);
  scene_destructor(&inference);
  inference = scene_constructor(false);
  scene_add_literal(inference, &wings);
  ck_assert_int_eq(knowledge_base->active->length, 2);
  ck_assert_float_eq_tol(wings_eagle->weight, 2, 0.000001);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2,
                               false, headers, header_size, incompatibilities);
  ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
  ck_assert_float_eq_tol(eagle_wings->weight, 3, 0.000001);
  ck_assert_float_eq_tol(wings_eagle->weight, 3, 0.000001);
  ck_assert_int_eq(knowledge_base->active->length, 3);
  ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
  ck_assert_rule_eq(knowledge_base->active->rules[2], wings_eagle);

  scene_add_literal(observation, &wings);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2,
                               false, headers, header_size, incompatibilities);
  ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
  ck_assert_float_eq_tol(eagle_wings->weight, 4, 0.000001);
  ck_assert_float_eq_tol(wings_eagle->weight, 4, 0.000001);
  ck_assert_int_eq(knowledge_base->active->length, 3);
  ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
  ck_assert_rule_eq(knowledge_base->active->rules[2], wings_eagle);

  knowledge_base_add_rule(knowledge_base, &wings_fly);
  knowledge_base_add_rule(knowledge_base, &penguin_n_fly);

  scene_destructor(&observation);
  observation = scene_constructor(false);
  scene_add_literal(observation, &wings);
  scene_add_literal(observation, &fly);
  scene_destructor(&inference);
  inference = scene_constructor(false);
  ck_assert_int_eq(knowledge_base->active->length, 4);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2,
                               false, headers, header_size, incompatibilities);
  ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
  ck_assert_float_eq_tol(eagle_wings->weight, 4, 0.000001);
  ck_assert_float_eq_tol(wings_eagle->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_n_fly->weight, 3, 0.000001);
  ck_assert_float_eq_tol(wings_fly->weight, 3, 0.000001);
  ck_assert_int_eq(knowledge_base->active->length, 5);
  ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
  ck_assert_rule_eq(knowledge_base->active->rules[2], wings_eagle);
  ck_assert_rule_eq(knowledge_base->active->rules[3], penguin_n_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[4], wings_fly);

  scene_destructor(&observation);
  observation = scene_constructor(false);
  scene_add_literal(observation, &wings);
  scene_add_literal(observation, &n_fly);
  scene_add_literal(observation, &penguin);
  scene_add_literal(inference, &n_fly);
  ck_assert_int_eq(knowledge_base->active->length, 5);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2,
                               false, headers, header_size, incompatibilities);
  ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
  ck_assert_float_eq_tol(eagle_wings->weight, 4, 0.000001);
  ck_assert_float_eq_tol(wings_eagle->weight, 2, 0.000001);
  ck_assert_float_eq_tol(penguin_n_fly->weight, 4, 0.000001);
  ck_assert_float_eq_tol(wings_fly->weight, 3, 0.000001);
  ck_assert_int_eq(knowledge_base->active->length, 4);
  ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
  ck_assert_rule_eq(knowledge_base->active->rules[2], penguin_n_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[3], wings_fly);

  scene_destructor(&observation);
  observation = scene_constructor(false);
  scene_add_literal(observation, &wings);
  scene_add_literal(observation, &n_fly);
  scene_add_literal(observation, &penguin);
  scene_destructor(&inference);
  inference = scene_constructor(false);
  ck_assert_int_eq(knowledge_base->active->length, 4);
  rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2,
                               false, headers, header_size, incompatibilities);
  ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
  ck_assert_float_eq_tol(eagle_wings->weight, 4, 0.000001);
  ck_assert_float_eq_tol(penguin_n_fly->weight, 5, 0.000001);
  ck_assert_float_eq_tol(wings_fly->weight, 1, 0.000001);
  ck_assert_int_eq(knowledge_base->active->length, 3);
  ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
  ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
  ck_assert_rule_eq(knowledge_base->active->rules[2], penguin_n_fly);

  scene_destructor(&observation);
  scene_destructor(&inference);
  scene_destructor(&birds);
  scene_destructor(&fly_labels);
  scene_destructor(&wings_labels);
  literal_destructor(&n_wings);
  knowledge_base_destructor(&knowledge_base);
}
END_TEST

Suite *rule_hypergraph_suite() {
  Suite *suite;
  TCase *create_case, *add_edges_case, *adding_and_removing_case,
      *get_inactive_case, *copy_case, *rule_update_case;

  suite = suite_create("Rule Hypergraph");
  create_case = tcase_create("Create");
  tcase_add_test(create_case, construct_destruct_hypergraph_test);
  tcase_add_test(create_case, construct_destruct_vector_test);
  tcase_add_test(create_case, construct_destruct_edge_test);
  suite_add_tcase(suite, create_case);

  add_edges_case = tcase_create("Add Edges");
  tcase_add_test(add_edges_case, adding_edges_test);
  suite_add_tcase(suite, add_edges_case);

  adding_and_removing_case = tcase_create("Adding and Removing Rules");
  tcase_add_test(adding_and_removing_case, adding_rules_test);
  tcase_add_test(adding_and_removing_case, removing_rules_test);
  suite_add_tcase(suite, adding_and_removing_case);

  get_inactive_case = tcase_create("Get Inactive Rules");
  tcase_add_test(get_inactive_case, get_inactive_rules_test);
  suite_add_tcase(suite, get_inactive_case);

  copy_case = tcase_create("Copy");
  tcase_add_test(copy_case, hypergraph_copy_test);
  suite_add_tcase(suite, copy_case);

  rule_update_case = tcase_create("Rule Update");
  tcase_add_test(rule_update_case, update_rules_test);
  suite_add_tcase(suite, rule_update_case);

  return suite;
}

int main() {
  Suite *suite = rule_hypergraph_suite();
  SRunner *s_runner;

  s_runner = srunner_create(suite);
  srunner_set_fork_status(s_runner, CK_NOFORK);

  srunner_run_all(s_runner, CK_ENV);
  int number_failed = srunner_ntests_failed(s_runner);
  srunner_free(s_runner);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif
