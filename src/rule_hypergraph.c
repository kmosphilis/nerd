#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <prb.h>

#include "nerd_utils.h"
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
// To do that we will need an additional field which will hold the vertices that a vertex is being
// used because if we delete a vertex being used, we will run into segmentation faults.

/**
 * @brief Destructs the given Edge and the Rule that resides within.
 *
 * @param edge The Edge to be destructed. It should be reference to an Edge * (Edge ** - a pointer
 * to an Edge *). Upon succession, this parameter will become NULL.
*/
void edge_destructor(Edge ** const edge) {
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
Vertex *vertex_constructor(Literal * const literal) {
    Vertex *vertex = (Vertex *) malloc(sizeof(Vertex));
    vertex->literal = literal;
    vertex->edges = NULL;
    vertex->number_of_edges = 0;
    return vertex;
}

/**
 * @brief Destructs the given Vertex.
 *
 * @param vertex The Vertex to be destructed. It should be reference to a Vertex * (Vertex ** - a
 * pointer to a Vertex *). Upon succession, this parameter will become NULL.
 * @param destruct_literal If true is given, the Literal within the vertrex will be destructed, and
 * if false is given it will not.
*/
void vertex_destructor(Vertex ** const vertex, const bool destruct_literal) {
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
 * @param rule_hypergraph The RuleHyperGraph to find the corresponding Vertices to be connected.
 * @param rule The Rule to show which Literals should be connected. The Body of a rule has the
 * origin Vertices, and the head has the destination Vertex. It should be reference to a Rule *
 * (Rule ** - a pointer to a Rule *). If the given Rule had took ownership of its content, a new
 * Rule will be created without taking the ownership of that content as it will belong to the
 * internal RB-Tree. If it does not take the ownership of the its content, the content pointers
 * might stil change, as the body and head will take the pointer of the item allocated in the
 * RB-Tree.
 * @param head_vertex The Vertex that the edge will be created for. It will not be added to the
 * Vertex.
 *
 * @return A new Edge *. Use edge_destructor to deallocate it.
*/
Edge *edge_constructor(RuleHyperGraph * const rule_hypergraph, Rule ** const rule,
Vertex * const head_vertex) {
    if (!rule) {
        return NULL;
    }

    Edge *edge = (Edge *) malloc(sizeof(Edge));
    edge->from = (Vertex **) malloc(sizeof(Vertex *) * (*rule)->body->size);
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
        Rule *new_rule = rule_constructor((*rule)->body->size, body, &(head_vertex->literal),
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
 * @brief Adds an Edge to a Vertex. The Vertex is the head of the Rule, and the Edge contains the
 * origin (body) of the Rule.
 *
 * @param vertex The Vertex to add the Edge to.
 * @param edge The Edge to be added to the Vertex.
*/
void vertex_add_edge(Vertex * const vertex, Edge * const edge) {
    if (vertex && edge) {
        vertex->edges = (Edge **) realloc(vertex->edges,
        sizeof(Edge *) * ++vertex->number_of_edges);
        vertex->edges[vertex->number_of_edges - 1] = edge;
    }
}

/**
 * @brief Removes an Edge from a Vertex.
 *
 * @param vertex The Vertex to remove the Edge from.
 * @param index The index of the Edge to be removed. If > vertex.number_of_edges, it will fail.
*/
void vertex_remove_edge(Vertex * const vertex, unsigned int index) {
    if (vertex && (index < vertex->number_of_edges)) {
        edge_destructor(&(vertex->edges[index]));
        --vertex->number_of_edges;
        if (vertex->number_of_edges == 0) {
            safe_free(vertex->edges);
            return;
        }

        Edge **old = vertex->edges;
        vertex->edges = (Edge **) malloc(sizeof(Edge *) * vertex->number_of_edges);

        if (index == 0) {
            memcpy(vertex->edges, old + 1, sizeof(Edge *) * vertex->number_of_edges);
        } else if (index == vertex->number_of_edges) {
            memcpy(vertex->edges, old, sizeof(Edge *) * vertex->number_of_edges);
        } else {
            memcpy(vertex->edges, old, sizeof(Edge *) * index);
            memcpy(vertex->edges + index, old + index + 1, sizeof(Edge *) *
            (vertex->number_of_edges - index));
        }

        free(old);
    }
}

/**
 * @brief Function to be used with the RB-Tree to deallocate the Vertices and the Edges.
*/
void item_destructor(void *item, void *param) {
    Vertex *v = (Vertex *) item;
    vertex_destructor(&v, true);
}

/**
 * @brief Function to be used with the RB-Tree to find the appropriate location of a Vertex through
 * comparison.
*/
int compare_literals(const void *vertex1, const void *vertex2, void *extra) {
    const Vertex *v1 = (Vertex *) vertex1, *v2 = (Vertex *) vertex2;
    char *l1_string = literal_to_string(v1->literal), *l2_string = literal_to_string(v2->literal);
    int result = strcmp(l1_string, l2_string);

    free(l1_string);
    free(l2_string);

    return result;
}

/**
 * @brief Costructs an empty RuleHyperGraph with an empty RB-Tree.
 *
 * @param use_backward_chaining A boolean value which indicates whether the hypergraph should demoted
 * rules using the backward chaining algorithm or not.
 *
 * @return A new RuleHyperGraph *. Use rule_hypergraph_destructor to deallocate.
*/
RuleHyperGraph *rule_hypergraph_empty_constructor(const bool use_backward_chaining) {
    RuleHyperGraph *hypergraph = (RuleHyperGraph *) malloc(sizeof(RuleHyperGraph));

    hypergraph->literal_tree = prb_create(compare_literals, NULL, NULL);
    hypergraph->use_backward_chaining = use_backward_chaining;

    return hypergraph;
}

/**
 * @brief Destructs the given RuleHyperGraph and its RB-Tree.
 *
 * @param rule_hypergraph The RuleHyperGraph to be destructed. It should be reference to a
 * RuleHyperGraph (RuleHyperGraph ** - a pointer to a RuleHyperGraph *). Upon succession, this
 * parameter will become NULL.
*/
void rule_hypergraph_destructor(RuleHyperGraph ** const rule_hypergraph) {
    if (rule_hypergraph && *rule_hypergraph && (*rule_hypergraph)->literal_tree) {
        prb_destroy((*rule_hypergraph)->literal_tree, item_destructor);
        safe_free(*rule_hypergraph);
    }
}

/**
 * @brief Makes a copy of the RuleHyperGraph in the given KnowledgeBase. If any of the parameters is
 * NULL, the process will fail.
 *
 * @param destination The KnowledgeBase to save the copy. It should be a reference to the struct's
 * pointer (to a KnowledgeBase * - &(KnowledgeBase *)).
 * @param source The KnowledgeBase to be copied.
*/
void rule_hypergraph_copy(KnowledgeBase ** const destination,
const KnowledgeBase * const source) {
    if (destination && source) {
        (*destination)->hypergraph =
        rule_hypergraph_empty_constructor(source->hypergraph->use_backward_chaining);

        struct prb_traverser traverser;

        Vertex *current_vertex = prb_t_first(&traverser, source->hypergraph->literal_tree);

        unsigned int i, j;
        while (current_vertex) {
            for (i = 0; i < current_vertex->number_of_edges; ++i) {
                Literal *head, **body =
                (Literal **) malloc(sizeof(Literal *) * current_vertex->edges[i]->rule->body->size);
                literal_copy(&head, current_vertex->edges[i]->rule->head);
                for(j = 0; j < current_vertex->edges[i]->rule->body->size; ++j) {
                    literal_copy(&(body[j]), current_vertex->edges[i]->rule->body->literals[j]);
                }
                Rule *rule =
                rule_constructor(j, body, &head, current_vertex->edges[i]->rule->weight, true);
                free(body);

                knowledge_base_add_rule(*destination, &rule);
            }
            current_vertex = prb_t_next(&traverser);
        }
    }
}

/**
 * @brief Adds a Rule to the given RuleHyperGraph. This process creates the appropriate Vertices and
 * an Edge to connected them.
 *
 * @param rule_hypergraph The RuleHyperGraph to add the Rule.
 * @param rule The Rule to be added to the RuleHyperGraph. If the given Rule had took ownership of
 * its content, a new Rule will be created without taking the ownership of that content as it will
 * belong to the internal RB-Tree. If it does not take the ownership of the its content, the content
 * pointers might stil change, as the body and head will take the pointer of the item allocated in
 * the RB-Tree.
 *
 * @return 1 if Rule was successfully added, 0 if it was not, and -1 if one of the parameters was
 * NULL.
*/
int rule_hypergraph_add_rule(RuleHyperGraph * const rule_hypergraph, Rule ** const rule) {
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
 * @brief Removes a Rule from the given RuleHyperGraph. This process only deletes the connecting
 * Edges, but leaves the Vertices involved unaffected (could change in the future).
 *
 * @param rule_hypergraph The RuleHyperGraph to remove the Rule from.
 * @param rule The Rule to be removed.
*/
void rule_hypergraph_remove_rule(RuleHyperGraph * const rule_hypergraph, Rule * const rule) {
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
 * @brief Finds all the inactive Rules in the RuleHyperGraph inside the KnowledgeBase.
 *
 * @param knowledge_base The KnowledgeBase which has the RuleHyperGraph to find the inactive Rules
 * from.
 * @param inactive_rules The output of the function to be returned. If NULL, the function will not
 * be performed. It should be a reference to the struct's pointer (RuleQueue **). Make sure that the
 * given double pointer is not already allocated, otherwise its contents will be lost in the memory.
 * The RuleQueue which will be created will not have ownership of its Rules.
*/
void rule_hypergraph_get_inactive_rules(const KnowledgeBase * const knowledge_base,
RuleQueue ** const inactive_rules) {
    if (knowledge_base && inactive_rules) {
        *inactive_rules = rule_queue_constructor(false);
        struct prb_traverser traverser;
        Vertex *result = (Vertex *) prb_t_first(&traverser,
        knowledge_base->hypergraph->literal_tree);
        unsigned int i;
        while (result) {
            for (i = 0; i < result->number_of_edges; ++i) {
                Rule *current_rule = result->edges[i]->rule;

                if (current_rule->weight < knowledge_base->activation_threshold) {
                    rule_queue_enqueue(*inactive_rules, &(current_rule));
                }
            }
            result = (Vertex *) prb_t_next(&traverser);
        }
    }
}

/**
 * @brief Updates the weight of (Promotes or Demotes) each rule according to the given observations
 * and inferences.
 *
 * Currently backward chaining demotion is the only option.
 *
 * @param knowledge_base The KnowledgeBase that contains the rules to be updated.
 * @param observations A Scene that contains all the observations that were encountered.
 * @param inferences A Scene that contains all the inferences that were made by a forward chaining
 * algorithm. Remember to include all the opposing inferences too, even if they are defeated by
 * default (conclusions that oppose observations are by default not valid).
 * @param promotion_rate The rate a rule should be promoted.
 * @param demotion_rate The rate a rule should be demoted.
 * @param increasing_demotion A boolean value which indicates whether the chaining demotion should
 * be increasing or not. It only works if and only if backward chaining demotion is enabled.
 * @param header (Optional) A char ** containing the header of a file to compare attributes. The
 * rest two optional parameters should be given together.
 * @param header_size (Optional) The size_t of the header.
 * @param incompatibilities (Optional) A Scene ** containing the incompatible literals corresponding
 * to each header. It should have the same size, even if no incompatible literals are given.
*/
void rule_hypergraph_update_rules(KnowledgeBase * const knowledge_base,
const Scene * const observations, const Scene * const inferences, const float promotion_rate,
const float demotion_rate, const bool increasing_demotion, char **header,
const size_t header_size, Scene **incompatibilities) {
    if (!(knowledge_base && observations && inferences)) {
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

    scene_union(observations, inferences, &observed_and_inferred);
    scene_difference(observations, inferences, &observed_diff_inferred);
    for (i = 0; i < header_size; ++i) {
        for (j = 0; j < observed_diff_inferred->size; ++j) {
            if (strstr(observed_diff_inferred->literals[j]->atom, header[i])) {
                for (k = 0; k < incompatibilities[i]->size; ++k) {
                    Literal *current_literal = incompatibilities[i]->literals[k], *copy;

                    if (literal_equals(current_literal, observed_diff_inferred->literals[j]) == 0) {
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

    // Finds all the Rules that concur by finding the observed Literal in the RB-tree.
    for (j = 0; j < observations->size; ++j) {
        vertex_to_find = vertex_constructor(observations->literals[j]);
        current_vertex = prb_find(knowledge_base->hypergraph->literal_tree, vertex_to_find);
        vertex_destructor(&vertex_to_find, false);

        if (current_vertex && (current_vertex->number_of_edges != 0)) {
            for (i = 0; i < current_vertex->number_of_edges; ++i) {
                current_rule = current_vertex->edges[i]->rule;
                // Checks if the Rule is applicable given the union of observed and inferred
                // Literals.
                if (rule_applicable(current_rule, observed_and_inferred)) {
                    bool is_inactive = current_rule->weight < knowledge_base->activation_threshold;
                    current_rule->weight += promotion_rate;

                    if (is_inactive &&
                    (current_rule->weight >= knowledge_base->activation_threshold)) {
                        rule_queue_enqueue(knowledge_base->active, &current_rule);
                    }
                }
            }
        }
    }

    Edge *current_edge = NULL;

    for (j = 0; j < opposing_literals->size; ++j) {
        vertex_to_find = vertex_constructor(opposing_literals->literals[j]);
        current_vertex = prb_find(knowledge_base->hypergraph->literal_tree, vertex_to_find);
        vertex_destructor(&vertex_to_find, false);

        if (current_vertex && (current_vertex->number_of_edges != 0)) {
            if (!knowledge_base->hypergraph->use_backward_chaining) {
                for (i = 0; i < current_vertex->number_of_edges; ++i) {
                    current_rule = current_vertex->edges[i]->rule;
                    // Checks if the Rule is applicable given the inferred and observed Literals.
                    bool was_active = current_rule->weight >= knowledge_base->activation_threshold;
                    if (rule_applicable(current_rule, observed_and_inferred)) {
                        current_rule->weight -= demotion_rate;

                        if (was_active &&
                        (current_rule->weight < knowledge_base->activation_threshold)) {
                            rule_queue_remove_rule(knowledge_base->active,
                            rule_queue_find(knowledge_base->active, current_rule), NULL);
                        }
                        if (current_rule->weight <= 0) {
                            vertex_remove_edge(current_vertex, i);
                            --i;
                        }
                    }
                }
                goto finished;
            }

            IntVector *vertex_edge_position = int_vector_constructor(),
            *edges_per_vertex = int_vector_constructor();

            Vertex **vertex_array = (Vertex **) malloc(sizeof(Vertex *));
            int_vector_push(vertex_edge_position, 0);
            int_vector_push(edges_per_vertex, 0);
            vertex_array[0] = current_vertex;
            size_t current_vertex_position;
            int *current_edge_position = NULL;

            // FIXME start again. Make the do while loop do eveything at depth 1, and the for depth n.
            do {
                current_vertex_position = vertex_edge_position->size - 1;
                current_vertex = vertex_array[current_vertex_position];
                current_edge_position = &(vertex_edge_position->items[current_vertex_position]);
                current_edge = current_vertex->edges[(*current_edge_position)];

                current_rule = current_edge->rule;

                bool is_inactive = current_rule->weight < knowledge_base->activation_threshold;

                // If the Rule was inactive and its head does not have the opposing Literal, it
                // should not be considered.
                if (is_inactive && (vertex_edge_position->size > 1)) {
                    goto edge_checking;
                }

                // Checks if a Rule is applicable.
                if (rule_applicable(current_rule, observed_and_inferred)) {
                    bool was_active = current_rule->weight >= knowledge_base->activation_threshold;
                    size_t depth = 1;

                    // Finds the correct depth/level of inference (in regards to the tree's depth).
                    for (i = 0; i < edges_per_vertex->size - 1;) {
                        depth += 1;
                        i += edges_per_vertex->items[i];
                    }

                    if (increasing_demotion) {
                        current_rule->weight -= demotion_rate * depth;
                    } else {
                        current_rule->weight -= demotion_rate / depth;
                    }

                    unsigned int k;
                    size_t total_inserted = 0, old_size = vertex_edge_position->size;

                    // Loops around the Rule's body.
                    for (i = 0; i < current_edge->number_of_vertices; ++i) {
                        // Checks if the current Literal from the Rule's body has incoming rules.
                        if ((scene_literal_index(observations, current_edge->from[i]->literal) < 0)
                        && (current_edge->from[i]->number_of_edges > 0)) {
                            // Checks is the Literal was encountered before.
                            if (old_size > 1) {
                                for (k = 0; k < old_size; ++k) {
                                    if (current_edge->from[i] == vertex_array[k]) {
                                        goto end_of_edge_vertices_loop;
                                    }
                                }
                            }
                            int_vector_push(vertex_edge_position, 0);
                            current_edge_position =
                            &(vertex_edge_position->items[current_vertex_position]);
                            ++total_inserted;
                            vertex_array = (Vertex **) realloc(vertex_array, sizeof(Vertex *) *
                            vertex_edge_position->size);
                            vertex_array[vertex_edge_position->size - 1] =
                            current_edge->from[i];
                        }
end_of_edge_vertices_loop:
                    }

                    // Desides whether to deactivate or remove a Rule.
                    if (was_active &&
                    (current_rule->weight < knowledge_base->activation_threshold)) {
                        rule_queue_remove_rule(knowledge_base->active,
                        rule_queue_find(knowledge_base->active, current_rule), NULL);
                    }

                    if (current_rule->weight <= 0) {
                        vertex_remove_edge(current_vertex, *current_edge_position);
                        if ((*current_edge_position) != 0) {
                            --(*current_edge_position);
                        }
                    }

                    if (total_inserted != 0) {
                        int_vector_set(edges_per_vertex, edges_per_vertex->size - 1,
                        total_inserted);
                        int_vector_push(edges_per_vertex, 0);
                    }
                }
edge_checking:
                // Removes Vertices (Literals) that had all of their incoming Rules checked
                // (recursively).
                if ((((size_t) (*current_edge_position)) == (current_vertex->number_of_edges - 1))
                || (((size_t) (*current_edge_position)) == current_vertex->number_of_edges)) {
                    if (current_vertex_position == (vertex_edge_position->size - 1)) {
                        unsigned int temp_vertex_position;
                        size_t total_deleted = 0;
                        do {
                            temp_vertex_position = vertex_edge_position->size - 1;
                            int_vector_delete(vertex_edge_position, temp_vertex_position);
                            if (vertex_edge_position->size != 0) {
                                vertex_array = (Vertex **) realloc(vertex_array, sizeof(Vertex *) *
                                vertex_edge_position->size);
                            } else {
                                safe_free(vertex_array);
                            }
                        } while (vertex_array &&
                        ((vertex_array[vertex_edge_position->size - 1]->number_of_edges - 1 ==
                        (size_t) vertex_edge_position->items[vertex_edge_position->size - 1]) ||
                        (vertex_array[vertex_edge_position->size - 1]->number_of_edges ==
                        (size_t) vertex_edge_position->items[vertex_edge_position->size - 1])));

                        if (total_deleted ==
                        (size_t) edges_per_vertex->items[edges_per_vertex->size - 1]) {
                            int_vector_delete(edges_per_vertex, edges_per_vertex->size - 1);
                        } else {
                            int *edges_to_check;
                            while (total_deleted != 0) {
                                edges_to_check =
                                &(edges_per_vertex->items[edges_per_vertex->size - 1]);
                                if ((size_t) (*edges_to_check) > total_deleted) {
                                    total_deleted -= *edges_to_check;
                                    int_vector_delete(edges_per_vertex, edges_per_vertex->size - 1);
                                } else {
                                    *edges_to_check -= total_deleted;
                                    total_deleted = 0;
                                }
                            }
                        }
                    }
                } else {
                    ++(*current_edge_position);
                }
            } while (vertex_edge_position->size != 0);

            int_vector_destructor(&vertex_edge_position);
            int_vector_destructor(&edges_per_vertex);
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
 * @brief Checks two Vertices to determine if they are equal or not. Do not use the same pointer on
 * both parameters.
 *
 * @param X The first Vertex to compare.
 * @param Y The second Vertex to compare.
*/
#define ck_assert_vertex_eq(X, Y) do { \
    const Vertex * const _v1 = (X); \
    const Vertex * const _v2 = (Y); \
    ck_assert_ptr_nonnull(_v1); \
    ck_assert_ptr_nonnull(_v2); \
    ck_assert_ptr_ne(_v1, _v2); \
    ck_assert_ptr_ne(_v1->literal, _v2->literal); \
    ck_assert_literal_eq(_v1->literal, _v2->literal); \
    ck_assert_int_eq(_v1->number_of_edges, _v2->number_of_edges); \
    if (_v1->number_of_edges > 0) { \
        ck_assert_ptr_ne(_v1->edges, _v2->edges); \
        unsigned int i; \
        for (i = 0; i < _v1->number_of_edges; ++i) { \
            ck_assert_ptr_ne(_v1->edges[i], _v2->edges[i]); \
        } \
    } \
} while (0)

/**
 * @brief Checks two Edges to determine if they are equal or not. Do not use the same pointer on
 * both parameters.
 *
 * @param X The first Edge to compare.
 * @param Y The second Edge to compare.
*/
#define ck_assert_edge_eq(X, Y) do { \
    const Edge * const _e1 = (X); \
    const Edge * const _e2 = (Y); \
    ck_assert_ptr_nonnull(_e1); \
    ck_assert_ptr_nonnull(_e2); \
    ck_assert_ptr_ne(_e1, _e2); \
    ck_assert_ptr_ne(_e1->rule, _e2->rule); \
    ck_assert_rule_eq(_e1->rule, _e2->rule); \
    ck_assert_ptr_ne(_e1->rule->head, _e2->rule->head); \
    ck_assert_ptr_ne(_e1->rule->body, _e2->rule->body); \
    unsigned int i; \
    for (i = 0; i < _e1->rule->body->size; ++i) { \
        ck_assert_ptr_ne(_e1->rule->body->literals[i], _e2->rule->body->literals[i]); \
    } \
    ck_assert_int_eq(_e1->number_of_vertices, _e2->number_of_vertices); \
    ck_assert_ptr_ne(_e1->from, _e2->from); \
    for (i = 0; i < _e1->number_of_vertices; ++i) { \
        ck_assert_ptr_ne(_e1->from[i], _e2->from[i]); \
        ck_assert_vertex_eq(_e1->from[i], _e2->from[i]); \
    } \
} while (0)

#define ck_assert_rule_hypergraph_eq(X, Y) do { \
    const RuleHyperGraph * const _h1 = (X); \
    const RuleHyperGraph * const _h2 = (Y); \
    ck_assert_ptr_nonnull(_h1); \
    ck_assert_ptr_nonnull(_h2); \
    ck_assert_ptr_ne(_h1, _h2); \
    ck_assert_ptr_nonnull(_h1->literal_tree); \
    ck_assert_ptr_nonnull(_h2->literal_tree); \
    ck_assert_ptr_ne(_h1->literal_tree, _h2->literal_tree); \
    struct prb_traverser _h1_traverser, _h2_traverser; \
    ck_assert_int_eq(_h1->literal_tree->prb_count, _h2->literal_tree->prb_count); \
    Vertex *_h1_current_vertex = (Vertex *) prb_t_first(&_h1_traverser, _h1->literal_tree), \
    *_h2_current_vertex = (Vertex *) prb_t_first(&_h2_traverser, _h2->literal_tree); \
    unsigned int i; \
    while (_h1_current_vertex) { \
        ck_assert_vertex_eq(_h1_current_vertex, _h2_current_vertex); \
        for (i = 0; i < _h1_current_vertex->number_of_edges; ++i) { \
            ck_assert_edge_eq(_h1_current_vertex->edges[i], _h2_current_vertex->edges[i]); \
        } \
        _h1_current_vertex = (Vertex *)  prb_t_next(&_h1_traverser); \
        _h2_current_vertex = (Vertex *) prb_t_next(&_h2_traverser); \
    } \
} while (0)

#define _ck_assert_rule_hypergraph_empty(X, OP) do { \
    const RuleHyperGraph * const _h = (X); \
    ck_assert_ptr_nonnull(_h); \
    ck_assert_ptr_nonnull(_h->literal_tree); \
    _ck_assert_int(_h->literal_tree->prb_count, OP, 0); \
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
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *c1 = l1, *c2 = l2;

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
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true), *l_array[2] = {l1, l3}, *c1, *c2, *c3;
    literal_copy(&c1, l1);
    literal_copy(&c2, l2);
    literal_copy(&c3, l3);
    Vertex *v1 = vertex_constructor(l1), *v2 = vertex_constructor(l2), *v3 = vertex_constructor(l3),
    *v_array[2] = {v1, v3};
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
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true), *l_array[2] = {l1, l3};
    Vertex *v2 = vertex_constructor(l2), *v3 = vertex_constructor(l3);
    Rule *r1 = rule_constructor(1, &l1, &l2, 0, false),
    *r2 = rule_constructor(1, &l1, &l3, 0, false),
    *r3 = rule_constructor(2, l_array, &l2, 0, false);

    prb_insert(hypergraph->literal_tree, v2);
    prb_insert(hypergraph->literal_tree, v3);

    Edge *e1 = edge_constructor(hypergraph, &r1, v2), *e2 = edge_constructor(hypergraph, &r2, v3);

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
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true), *l4 = literal_constructor("fly", true),
    *l5 = literal_constructor("wings", true), *l_array[2] = {l3, l2}, *c1, *c2;
    literal_copy(&c1, l1);
    literal_copy(&c2, l2);
    Vertex *v1 = vertex_constructor(l1), *v2 = vertex_constructor(l2),
    *v3 = vertex_constructor(l3), *v4 = vertex_constructor(l4), *v5 = vertex_constructor(l5),
    *current_v1, *current_v2;
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
    ck_assert_literal_eq(current_v1->edges[0]->from[0]->edges[0]->from[0]->literal, l3);

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
    ck_assert_ptr_eq(current_v2->edges[0]->from[0], current_v1->edges[0]->from[0]);

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

    Literal *l6 = literal_constructor("chicken", true), *l7 = literal_constructor("feathers", true),
    *c6, *c7;
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
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true), *l4 = literal_constructor("fly", true),
    *l5 = literal_constructor("wings", true), *l_array[2] = {l3, l5};
    Vertex *v1 = vertex_constructor(l1), *v2 = vertex_constructor(l2),
    *v3 = vertex_constructor(l3), *v4 = vertex_constructor(l4), *v5 = vertex_constructor(l5),
    *current_v1, *current_v2;
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
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true), *l4 = literal_constructor("fly", true),
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
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true), *l4 = literal_constructor("fly", true),
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
    kb2 = (KnowledgeBase *) malloc(sizeof(KnowledgeBase));
    kb2->activation_threshold = kb1->activation_threshold;
    kb2->active = rule_queue_constructor(false);
    rule_hypergraph_copy(&kb2, kb1);
    ck_assert_ptr_nonnull(kb2->hypergraph);
    ck_assert_ptr_ne(kb1->hypergraph, kb2->hypergraph);
    ck_assert_ptr_ne(kb1->hypergraph->literal_tree, kb2->hypergraph->literal_tree);

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

START_TEST(update_rules_test) {
    KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0, false);
    Literal *peng = literal_constructor("penguin", true), *fly = literal_constructor("fly", true),
    *bird = literal_constructor("bird", true), *n_fly = literal_constructor("fly", false),
    *wings = literal_constructor("wings", true), *l_array[2] = {bird, wings};
    Vertex *v1 = vertex_constructor(peng), *v2 = vertex_constructor(fly),
    *v3 = vertex_constructor(bird), *v4 = vertex_constructor(n_fly), *v5 = vertex_constructor(wings);
    Rule *peng_fly = rule_constructor(1, &peng, &fly, 0, false),
    *bird_n_fly = rule_constructor(1, &bird, &n_fly, 0, false),
    *wings_n_fly = rule_constructor(1, &wings, &n_fly, 0, false),
    *wings_bird = rule_constructor(1, &wings, &bird, 0, false),
    *bird_wings_n_fly = rule_constructor(2, l_array, &n_fly, 0, false);
    l_array[1] = peng;
    Rule *bird_peng_fly = rule_constructor(2, l_array, &fly, 0, false);

    rule_hypergraph_add_rule(knowledge_base->hypergraph, &peng_fly);
    rule_hypergraph_add_rule(knowledge_base->hypergraph, &bird_n_fly);
    rule_hypergraph_add_rule(knowledge_base->hypergraph, &wings_n_fly);
    rule_hypergraph_add_rule(knowledge_base->hypergraph, &wings_bird);
    rule_hypergraph_add_rule(knowledge_base->hypergraph, &bird_wings_n_fly);
    rule_hypergraph_add_rule(knowledge_base->hypergraph, &bird_peng_fly);

    Scene *observation = scene_constructor(false), *inference = scene_constructor(false);

    ck_assert_float_eq_tol(peng_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(wings_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_peng_fly->weight, 0, 0.000001);
    ck_assert_rule_queue_empty(knowledge_base->active);

    RuleQueue *inactive_rules;

    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2, false, NULL, 0,
    NULL);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 6);
    rule_queue_destructor(&inactive_rules);
    ck_assert_float_eq_tol(peng_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(wings_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_peng_fly->weight, 0, 0.000001);
    ck_assert_rule_queue_empty(knowledge_base->active);

    scene_add_literal(observation, &peng);
    scene_add_literal(observation, &wings);
    ck_assert_float_eq_tol(peng_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(wings_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_peng_fly->weight, 0, 0.000001);
    ck_assert_rule_queue_empty(knowledge_base->active);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2, false, NULL, 0,
    NULL);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 6);
    rule_queue_destructor(&inactive_rules);
    ck_assert_float_eq_tol(peng_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(wings_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_peng_fly->weight, 0, 0.000001);
    ck_assert_rule_queue_empty(knowledge_base->active);

    scene_add_literal(observation, &fly);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2, false, NULL, 0,
    NULL);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 5);
    rule_queue_destructor(&inactive_rules);
    ck_assert_float_eq_tol(peng_fly->weight, 1.5, 0.000001);
    ck_assert_float_eq_tol(bird_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
    ck_assert_float_eq_tol(bird_peng_fly->weight, 0, 0.000001);
    ck_assert_rule_queue_empty(knowledge_base->active);

    scene_add_literal(observation, &bird);
    ck_assert_rule_queue_empty(knowledge_base->active);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2, false, NULL, 0,
    NULL);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 2);
    rule_queue_destructor(&inactive_rules);
    ck_assert_float_eq_tol(peng_fly->weight, 3.0, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 1.5, 0.000001);
    ck_assert_float_eq_tol(bird_peng_fly->weight, 1.5, 0.000001);
    ck_assert_rule_queue_notempty(knowledge_base->active);
    ck_assert_int_eq(knowledge_base->active->length, 1);
    ck_assert_rule_eq(knowledge_base->active->rules[0], peng_fly);

    scene_remove_literal(observation, observation->size - 1, NULL);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2, false, NULL, 0,
    NULL);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 2);
    rule_queue_destructor(&inactive_rules);
    ck_assert_float_eq_tol(peng_fly->weight, 4.5, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 1.5, 0.000001);
    ck_assert_float_eq_tol(bird_peng_fly->weight, 1.5, 0.000001);
    ck_assert_rule_queue_notempty(knowledge_base->active);
    ck_assert_int_eq(knowledge_base->active->length, 1);
    ck_assert_rule_eq(knowledge_base->active->rules[0], peng_fly);

    wings_n_fly = rule_constructor(1, &wings, &n_fly, 0, false);
    knowledge_base_add_rule(knowledge_base, &wings_n_fly);

    scene_remove_literal(observation, observation->size - 1, NULL);
    scene_add_literal(observation, &n_fly);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2, false, NULL, 0,
    NULL);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 4);
    rule_queue_destructor(&inactive_rules);
    ck_assert_float_eq_tol(peng_fly->weight, 2.5, 0.000001);
    ck_assert_float_eq_tol(wings_n_fly->weight, 1.5, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 1.5, 0.000001);
    ck_assert_float_eq_tol(bird_peng_fly->weight, 1.5, 0.000001);
    ck_assert_rule_queue_empty(knowledge_base->active);

    bird_n_fly = rule_constructor(1, &bird, &n_fly, 0, false);
    knowledge_base_add_rule(knowledge_base, &bird_n_fly);
    l_array[0] = bird;
    l_array[1] = wings;
    bird_wings_n_fly = rule_constructor(2, l_array, &n_fly, 0, false);
    knowledge_base_add_rule(knowledge_base, &bird_wings_n_fly);

    scene_remove_literal(observation, 1, NULL);
    scene_add_literal(observation, &bird);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 2.25, 2, false, NULL, 0,
    NULL);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 5);
    rule_queue_destructor(&inactive_rules);
    ck_assert_float_eq_tol(peng_fly->weight, 0.5, 0.000001);
    ck_assert_float_eq_tol(bird_n_fly->weight, 2.25, 0.000001);
    ck_assert_float_eq_tol(wings_n_fly->weight, 1.5, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 1.5, 0.000001);
    ck_assert_float_eq_tol(bird_wings_n_fly->weight, 0, 0.000001);
    ck_assert_rule_queue_empty(knowledge_base->active);

    scene_add_literal(observation, &wings);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2, false, NULL, 0,
    NULL);
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
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1.5, 2, false, NULL, 0,
    NULL);
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
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2, false, NULL, 0,
    NULL);
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
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2, false, NULL, 0,
    NULL);
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
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2, false, NULL, 0,
    NULL);
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
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2, false, NULL, 0,
    NULL);
    ck_assert_float_eq_tol(wings_n_fly->weight, 1.0, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 3.5, 0.000001);
    ck_assert_rule_queue_notempty(knowledge_base->active);
    ck_assert_int_eq(knowledge_base->active->length, 1);
    ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);

    scene_remove_literal(observation, 0, NULL);
    scene_add_literal(observation, &wings);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2, false, NULL, 0,
    NULL);
    ck_assert_float_eq_tol(wings_n_fly->weight, 1.0, 0.000001);
    ck_assert_float_eq_tol(wings_bird->weight, 3.5, 0.000001);
    ck_assert_rule_queue_notempty(knowledge_base->active);
    ck_assert_int_eq(knowledge_base->active->length, 1);
    ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);

    scene_remove_literal(observation, 0, NULL);
    scene_remove_literal(inference, 0, NULL);
    scene_add_literal(observation, &fly);
    scene_add_literal(inference, &wings);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 2, false, NULL, 0,
    NULL);
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
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, NULL, 0,
    NULL);
    ck_assert_float_eq_tol(wings_bird->weight, 3.5, 0.000001);
    ck_assert_float_eq_tol(wings_fly->weight, 2.5, 0.000001);
    ck_assert_rule_queue_notempty(knowledge_base->active);
    ck_assert_int_eq(knowledge_base->active->length, 1);
    ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);

    // scene_add_literal(observation, &bird);
    // wings_n_fly->weight = 5.0;
    // wings_bird->weight = 5.0;
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 5.25, 0.000001);
    // ck_assert_float_eq_tol(bird_n_fly->weight, 1.5, 0.000001);
    // ck_assert_float_eq_tol(wings_n_fly->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 5.5, 0.000001);
    // ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.0, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 3.75, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 4);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], wings_bird);

    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_add_literal(observation, &n_fly);
    // scene_add_literal(inference, &bird);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 5.25, 0.000001);
    // ck_assert_float_eq_tol(bird_n_fly->weight, 0.5, 0.000001);
    // ck_assert_float_eq_tol(wings_n_fly->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 5.5, 0.000001);
    // ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.0, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 3.75, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 4);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], wings_bird);

    // Rule *fly_peng = rule_constructor(1, &fly, &peng, 5.5, false);
    // knowledge_base_add_rule(knowledge_base, &fly_peng);

    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // Literal *n_peng;
    // literal_copy(&n_peng, peng);
    // literal_negate(n_peng);
    // scene_add_literal(observation, &wings);
    // scene_add_literal(observation, &n_peng);
    // scene_add_literal(inference, &peng);
    // scene_add_literal(inference, &fly);
    // scene_add_literal(inference, &bird);
    // ck_assert_int_eq(knowledge_base->active->length, 5);
    // ck_assert_rule_eq(knowledge_base->active->rules[4], fly_peng);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 4.75, 0.000001);
    // ck_assert_float_eq_tol(bird_n_fly->weight, 0.5, 0.000001);
    // ck_assert_float_eq_tol(wings_n_fly->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 5.166667, 0.000001);
    // ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.0, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 3.25, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 4.5, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 5);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[4], fly_peng);


    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // Literal *eagle = literal_constructor("eagle", true), *n_eagle;
    // literal_copy(&n_eagle, eagle);
    // literal_negate(n_eagle);
    // scene_add_literal(observation, &n_eagle);
    // scene_add_literal(inference, &wings);
    // scene_add_literal(inference, &peng);
    // scene_add_literal(inference, &eagle);
    // l_array[0] = wings;
    // l_array[1] = peng;
    // Rule *wings_peng_eagle = rule_constructor(2, l_array, &eagle, 5.5, false);
    // knowledge_base_add_rule(knowledge_base, &wings_peng_eagle);
    // ck_assert_float_eq_tol(wings_peng_eagle->weight, 5.5, 0.000001);
    // ck_assert_int_eq(knowledge_base->active->length, 6);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 4.75, 0.000001);
    // ck_assert_float_eq_tol(bird_n_fly->weight, 0.5, 0.000001);
    // ck_assert_float_eq_tol(wings_n_fly->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 5.166667, 0.000001);
    // ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.0, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 3.25, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 4.5, 0.000001);
    // ck_assert_float_eq_tol(wings_peng_eagle->weight, 4.5, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 6);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[4], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[5], wings_peng_eagle);

    // scene_add_literal(inference, &fly);
    // scene_add_literal(inference, &bird);
    // bird_peng_fly->weight = 5;
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 4.416667, 0.000001);
    // ck_assert_float_eq_tol(bird_n_fly->weight, 0.5, 0.000001);
    // ck_assert_float_eq_tol(wings_n_fly->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 4.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.0, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 4.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_peng_eagle->weight, 3.5, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 6);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[4], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[5], wings_peng_eagle);

    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // scene_add_literal(observation, &n_fly);
    // scene_add_literal(inference, &bird);
    // scene_add_literal(inference, &n_fly);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // size_t old_inactives_size = inactive_rules->length;
    // rule_queue_destructor(&inactive_rules);

    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, NULL);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // ck_assert_int_ne(old_inactives_size, inactive_rules->length);
    // rule_queue_destructor(&inactive_rules);
    // ck_assert_float_eq_tol(peng_fly->weight, 4.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_n_fly->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 4.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.0, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 4.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_peng_eagle->weight, 3.5, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 6);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[4], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[5], wings_peng_eagle);

    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 4.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_n_fly->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 4.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_wings_n_fly->weight, 1.0, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 4.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 4.0, 0.000001);
    // ck_assert_float_eq_tol(wings_peng_eagle->weight, 3.5, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 6);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[4], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[5], wings_peng_eagle);

    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // scene_add_literal(observation, &n_fly);
    // scene_add_literal(observation, &n_eagle);
    // scene_add_literal(inference, &n_fly);
    // scene_add_literal(inference, &eagle);
    // scene_add_literal(inference, &wings);
    // scene_add_literal(inference, &peng);
    // scene_add_literal(inference, &bird);
    // Vertex *v6 = vertex_constructor(eagle);
    // Vertex *result = (Vertex *) prb_find(knowledge_base->hypergraph->literal_tree, v6);
    // size_t old_edge_size = v6->number_of_edges;

    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, NULL);
    // ck_assert_int_ne(old_edge_size, result->number_of_edges);
    // ck_assert_float_eq_tol(peng_fly->weight, 4.916667, 0.000001);
    // ck_assert_float_eq_tol(wings_n_fly->weight, 3.0, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 4.416667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 5.166667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 3.5, 0.000001);
    // ck_assert_float_eq_tol(wings_peng_eagle->weight, 2.5, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 5);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_n_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[4], fly_peng);

    // scene_add_literal(observation, &wings);
    // scene_add_literal(observation, &bird);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // old_inactives_size = inactive_rules->length;
    // Rule *old_rule_ptr = inactive_rules->rules[0];
    // rule_queue_destructor(&inactive_rules);

    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, NULL);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // ck_assert_int_eq(old_inactives_size, inactive_rules->length);
    // ck_assert_ptr_ne(old_rule_ptr, inactive_rules->rules[0]);
    // rule_queue_destructor(&inactive_rules);
    // ck_assert_float_eq_tol(peng_fly->weight, 5.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 4.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 5.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 2.0, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], wings_bird);

    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 5.916667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 5.416667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 6.166667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 2.0, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], wings_bird);

    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 5.916667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 5.416667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 6.166667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 2.0, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], wings_bird);

    // Rule *fly_bird = rule_constructor(1, &fly, &bird, 5.0, false);
    // Literal *n_bird;
    // literal_copy(&n_bird, bird);
    // literal_negate(n_bird);
    // knowledge_base_add_rule(knowledge_base, &fly_bird);
    // scene_add_literal(observation, &peng);
    // scene_add_literal(observation, &n_fly);
    // scene_add_literal(observation, &n_bird);
    // scene_add_literal(inference, &peng);
    // scene_add_literal(inference, &fly);
    // scene_add_literal(inference, &bird);
    // ck_assert_int_eq(knowledge_base->active->length, 4);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], fly_bird);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 6.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 5.416667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 6.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 2.5, 0.000001);
    // ck_assert_float_eq_tol(fly_bird->weight, 2.0, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], wings_bird);

    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // scene_add_literal(observation, &peng);
    // scene_add_literal(observation, &bird);
    // scene_add_literal(observation, &fly);
    // scene_add_literal(inference, &fly);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 3.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 5.416667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 3.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 3.0, 0.000001);
    // ck_assert_float_eq_tol(fly_bird->weight, 2.5, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 4);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bird_peng_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], fly_peng);

    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // scene_add_literal(observation, &peng);
    // scene_add_literal(observation, &wings);
    // scene_add_literal(observation, &fly);
    // scene_add_literal(inference, &fly);
    // scene_add_literal(inference, &bird);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 0.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 3.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 0.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 3.5, 0.000001);
    // ck_assert_float_eq_tol(fly_bird->weight, 2.5, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 2);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], fly_peng);


    // Literal *bat = literal_constructor_from_string("bat");
    // Vertex *v7 = vertex_constructor(bat);
    // Rule *bat_wings = rule_constructor(1, &bat, &wings, 12, false);
    // knowledge_base_add_rule(knowledge_base, &bat_wings);

    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // scene_add_literal(observation, &bat);
    // scene_add_literal(observation, &n_bird);
    // scene_add_literal(inference, &wings);
    // scene_add_literal(inference, &bird);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], wings_bird);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bat_wings);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, true, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 0.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 0.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 0.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 3.5, 0.000001);
    // ck_assert_float_eq_tol(fly_bird->weight, 2.5, 0.000001);
    // ck_assert_float_eq_tol(bat_wings->weight, 6, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 2);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bat_wings);

    // Literal *glide = literal_constructor("glide", true),
    // *flying_squir = literal_constructor("flying squirrel", true);
    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // scene_add_literal(observation, &glide);
    // scene_add_literal(observation, &flying_squir);
    // scene_add_literal(inference, &n_fly);
    // Rule *r11 = rule_constructor(1, &flying_squir, &n_fly, 3, false);
    // knowledge_base_add_rule(knowledge_base, &r11);
    // ck_assert_float_eq_tol(r11->weight, 3, 0.000001);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, NULL);
    // ck_assert_float_eq_tol(peng_fly->weight, 0.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 0.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 0.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 3.5, 0.000001);
    // ck_assert_float_eq_tol(fly_bird->weight, 2.5, 0.000001);
    // ck_assert_float_eq_tol(bat_wings->weight, 6, 0.000001);
    // ck_assert_float_eq_tol(r11->weight, 3, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bat_wings);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], r11);

    // Scene *labels = scene_constructor(false);
    // scene_add_literal(labels, &n_fly);
    // scene_add_literal(labels, &fly);
    // scene_add_literal(labels, &glide);
    // ck_assert_float_eq_tol(r11->weight, 3, 0.000001);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // old_inactives_size = inactive_rules->length;
    // rule_queue_destructor(&inactive_rules);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, labels);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // ck_assert_int_eq(old_inactives_size, inactive_rules->length);
    // rule_queue_destructor(&inactive_rules);
    // ck_assert_float_eq_tol(peng_fly->weight, 0.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 0.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 0.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 3.5, 0.000001);
    // ck_assert_float_eq_tol(fly_bird->weight, 2.5, 0.000001);
    // ck_assert_float_eq_tol(bat_wings->weight, 6, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 2);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bat_wings);

    // Rule *bat_n_fly = rule_constructor(1, &bat, &n_bird, 6, false),
    // *n_bird_fly = rule_constructor(1, &n_bird, &fly, 6, false);
    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_destructor(&inference);
    // inference = scene_constructor(false);
    // scene_add_literal(observation, &bat);
    // scene_add_literal(observation, &fly);
    // scene_add_literal(inference, &n_bird);
    // scene_add_literal(inference, &fly);
    // knowledge_base_add_rule(knowledge_base, &bat_n_fly);
    // knowledge_base_add_rule(knowledge_base, &n_bird_fly);
    // ck_assert_float_eq_tol(bat_n_fly->weight, 6, 0.000001);
    // ck_assert_float_eq_tol(n_bird_fly->weight, 6, 0.000001);
    // ck_assert_int_eq(knowledge_base->active->length, 4);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, labels);
    // ck_assert_float_eq_tol(peng_fly->weight, 0.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 0.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 0.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 3.5, 0.000001);
    // ck_assert_float_eq_tol(fly_bird->weight, 2.5, 0.000001);
    // ck_assert_float_eq_tol(bat_wings->weight, 6, 0.000001);
    // ck_assert_float_eq_tol(bat_n_fly->weight, 4.5, 0.000001);
    // ck_assert_float_eq_tol(n_bird_fly->weight, 3, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 4);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bat_wings);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bat_n_fly);
    // ck_assert_rule_eq(knowledge_base->active->rules[3], n_bird_fly);

    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // old_inactives_size = inactive_rules->length;
    // rule_queue_destructor(&inactive_rules);
    // knowledge_base->hypergraph->use_backward_chaining = false;
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 1, false, labels);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // ck_assert_int_lt(old_inactives_size, inactive_rules->length);
    // rule_queue_destructor(&inactive_rules);
    // ck_assert_float_eq_tol(peng_fly->weight, 0.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 0.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 0.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 3.5, 0.000001);
    // ck_assert_float_eq_tol(fly_bird->weight, 2.5, 0.000001);
    // ck_assert_float_eq_tol(bat_wings->weight, 6, 0.000001);
    // ck_assert_float_eq_tol(bat_n_fly->weight, 4.5, 0.000001);
    // ck_assert_float_eq_tol(n_bird_fly->weight, 2, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bat_wings);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bat_n_fly);

    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // old_inactives_size = inactive_rules->length;
    // rule_queue_destructor(&inactive_rules);
    // knowledge_base->hypergraph->use_backward_chaining = false;
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, labels);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // ck_assert_int_lt(inactive_rules->length, old_inactives_size);
    // rule_queue_destructor(&inactive_rules);
    // ck_assert_float_eq_tol(peng_fly->weight, 0.416667, 0.000001);
    // ck_assert_float_eq_tol(wings_bird->weight, 0.916667, 0.000001);
    // ck_assert_float_eq_tol(bird_peng_fly->weight, 0.666667, 0.000001);
    // ck_assert_float_eq_tol(fly_peng->weight, 3.5, 0.000001);
    // ck_assert_float_eq_tol(fly_bird->weight, 2.5, 0.000001);
    // ck_assert_float_eq_tol(bat_wings->weight, 6, 0.000001);
    // ck_assert_float_eq_tol(bat_n_fly->weight, 4.5, 0.000001);
    // ck_assert_rule_queue_notempty(knowledge_base->active);
    // ck_assert_int_eq(knowledge_base->active->length, 3);
    // ck_assert_rule_eq(knowledge_base->active->rules[0], fly_peng);
    // ck_assert_rule_eq(knowledge_base->active->rules[1], bat_wings);
    // ck_assert_rule_eq(knowledge_base->active->rules[2], bat_n_fly);

    // scene_destructor(&labels);
    scene_destructor(&observation);
    scene_destructor(&inference);
    vertex_destructor(&v1, false);
    vertex_destructor(&v2, false);
    vertex_destructor(&v3, false);
    vertex_destructor(&v4, false);
    vertex_destructor(&v5, false);
    // vertex_destructor(&v6, false);
    // vertex_destructor(&v7, false);
    // literal_destructor(&n_peng);
    // literal_destructor(&n_eagle);
    knowledge_base_destructor(&knowledge_base);

    knowledge_base = knowledge_base_constructor(3.0, false);

    peng = literal_constructor_from_string("bird_penguin"),
    fly = literal_constructor_from_string("fly_yes"),
    n_fly = literal_constructor_from_string("fly_no");
    Literal *eagle = literal_constructor_from_string("bird_eagle"),
    *n_wings = literal_constructor_from_string("wings_no");
    wings = literal_constructor_from_string("wings_yes");

    const size_t header_size = 3;
    char *header[] = {"bird", "fly", "wings"};
    Scene *birds = scene_constructor(false), *fly_labels = scene_constructor(false),
    *wings_labels = scene_constructor(false);
    scene_add_literal(birds, &peng);
    scene_add_literal(birds, &eagle);
    scene_add_literal(fly_labels, &fly);
    scene_add_literal(fly_labels, &n_fly);
    scene_add_literal(wings_labels, &wings);
    scene_add_literal(wings_labels, &n_wings);
    Scene *incompatibilities[] = {birds, fly_labels, wings_labels};

    Rule *eagle_fly = rule_constructor(1, &eagle, &fly, 3, false),
    *eagle_wings = rule_constructor(1, &eagle, &wings, 3, false);
    peng_fly = rule_constructor(1, &peng, &fly, 4, false);

    knowledge_base_add_rule(knowledge_base, &peng_fly);
    knowledge_base_add_rule(knowledge_base, &eagle_fly);
    knowledge_base_add_rule(knowledge_base, &eagle_wings);

    observation = scene_constructor(false);
    scene_add_literal(observation, &peng);
    scene_add_literal(observation, &n_fly);
    inference = scene_constructor(false);
    ck_assert_int_eq(knowledge_base->active->length, 3);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 0);
    rule_queue_destructor(&inactive_rules);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2, false, NULL,
    0, NULL);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 0);
    rule_queue_destructor(&inactive_rules);
    ck_assert_int_eq(knowledge_base->active->length, 3);
    ck_assert_rule_eq(knowledge_base->active->rules[0], peng_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[2], eagle_wings);

    ck_assert_int_eq(knowledge_base->active->length, 3);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 0);
    rule_queue_destructor(&inactive_rules);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2, false, header,
    header_size, incompatibilities);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 1);
    rule_queue_destructor(&inactive_rules);
    ck_assert_int_eq(knowledge_base->active->length, 2);
    ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);

    scene_remove_literal(observation, scene_literal_index(observation, n_fly), NULL);
    ck_assert_int_eq(knowledge_base->active->length, 2);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 1);
    rule_queue_destructor(&inactive_rules);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2, false, header,
    header_size, incompatibilities);
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
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2, false, header,
    header_size, incompatibilities);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, 1);
    rule_queue_destructor(&inactive_rules);
    ck_assert_int_eq(knowledge_base->active->length, 2);
    ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);

    wings_fly = rule_constructor(1, &wings, &fly, 2, false);
    Rule *peng_n_fly = rule_constructor(1, &peng, &n_fly, 3, false),
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
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2, false, header,
    header_size, incompatibilities);
    ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
    ck_assert_float_eq_tol(eagle_wings->weight, 3, 0.000001);
    ck_assert_float_eq_tol(wings_eagle->weight, 3, 0.000001);
    ck_assert_int_eq(knowledge_base->active->length, 3);
    ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
    ck_assert_rule_eq(knowledge_base->active->rules[2], wings_eagle);


    scene_add_literal(observation, &wings);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2, false, header,
    header_size, incompatibilities);
    ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
    ck_assert_float_eq_tol(eagle_wings->weight, 4, 0.000001);
    ck_assert_float_eq_tol(wings_eagle->weight, 4, 0.000001);
    ck_assert_int_eq(knowledge_base->active->length, 3);
    ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
    ck_assert_rule_eq(knowledge_base->active->rules[2], wings_eagle);

    knowledge_base_add_rule(knowledge_base, &wings_fly);
    knowledge_base_add_rule(knowledge_base, &peng_n_fly);

    scene_destructor(&observation);
    observation = scene_constructor(false);
    scene_add_literal(observation, &wings);
    scene_add_literal(observation, &fly);
    scene_destructor(&inference);
    inference = scene_constructor(false);
    ck_assert_int_eq(knowledge_base->active->length, 4);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2, false, header,
    header_size, incompatibilities);
    ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
    ck_assert_float_eq_tol(eagle_wings->weight, 4, 0.000001);
    ck_assert_float_eq_tol(wings_eagle->weight, 4, 0.000001);
    ck_assert_float_eq_tol(peng_n_fly->weight, 3, 0.000001);
    ck_assert_float_eq_tol(wings_fly->weight, 3, 0.000001);
    ck_assert_int_eq(knowledge_base->active->length, 5);
    ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
    ck_assert_rule_eq(knowledge_base->active->rules[2], wings_eagle);
    ck_assert_rule_eq(knowledge_base->active->rules[3], peng_n_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[4], wings_fly);

    scene_destructor(&observation);
    observation = scene_constructor(false);
    scene_add_literal(observation, &wings);
    scene_add_literal(observation, &n_fly);
    scene_add_literal(observation, &peng);
    scene_add_literal(inference, &n_fly);
    ck_assert_int_eq(knowledge_base->active->length, 5);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2, false, header,
    header_size, incompatibilities);
    ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
    ck_assert_float_eq_tol(eagle_wings->weight, 4, 0.000001);
    ck_assert_float_eq_tol(wings_eagle->weight, 2, 0.000001);
    ck_assert_float_eq_tol(peng_n_fly->weight, 4, 0.000001);
    ck_assert_float_eq_tol(wings_fly->weight, 3, 0.000001);
    ck_assert_int_eq(knowledge_base->active->length, 4);
    ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
    ck_assert_rule_eq(knowledge_base->active->rules[2], peng_n_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[3], wings_fly);

    scene_destructor(&observation);
    observation = scene_constructor(false);
    scene_add_literal(observation, &wings);
    scene_add_literal(observation, &n_fly);
    scene_add_literal(observation, &peng);
    scene_destructor(&inference);
    inference = scene_constructor(false);
    ck_assert_int_eq(knowledge_base->active->length, 4);
    rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 2, false, header,
    header_size, incompatibilities);
    ck_assert_float_eq_tol(eagle_fly->weight, 3, 0.000001);
    ck_assert_float_eq_tol(eagle_wings->weight, 4, 0.000001);
    ck_assert_float_eq_tol(peng_n_fly->weight, 5, 0.000001);
    ck_assert_float_eq_tol(wings_fly->weight, 1, 0.000001);
    ck_assert_int_eq(knowledge_base->active->length, 3);
    ck_assert_rule_eq(knowledge_base->active->rules[0], eagle_fly);
    ck_assert_rule_eq(knowledge_base->active->rules[1], eagle_wings);
    ck_assert_rule_eq(knowledge_base->active->rules[2], peng_n_fly);

    scene_destructor(&observation);
    scene_destructor(&inference);
    scene_destructor(&birds);
    scene_destructor(&fly_labels);
    scene_destructor(&wings_labels);
    literal_destructor(&n_wings);
    knowledge_base_destructor(&knowledge_base);


    // peng = literal_constructor_from_string("peng");
    // bird = literal_constructor_from_string("bird");
    // fly = literal_constructor_from_string("fly");
    // wings = literal_constructor_from_string("wings");
    // Literal *feathers = literal_constructor_from_string("feathers");

    // knowledge_base = knowledge_base_constructor(3, false);
    // Rule *bird_fly = rule_constructor(1, &bird, &fly, 3, false),
    // *feathers_wings = rule_constructor(1, &feathers, &wings, 4, false),
    // *feathers_bird = rule_constructor(1, &feathers, &bird, 4, false);
    // knowledge_base_add_rule(knowledge_base, &bird_fly);
    // knowledge_base_add_rule(knowledge_base, &feathers_wings);
    // knowledge_base_add_rule(knowledge_base, &feathers_bird);
    // l_array[0] = bird;
    // l_array[1] = peng;
    // bird_peng_fly = rule_constructor(2, l_array, &fly, 4, false);
    // knowledge_base_add_rule(knowledge_base, &bird_peng_fly);
    // peng_fly = rule_constructor(1, &peng, &fly, 4, false);
    // knowledge_base_add_rule(knowledge_base, &peng_fly);
    // wings_bird = rule_constructor(1, &wings, &bird, 4, false);
    // knowledge_base_add_rule(knowledge_base, &wings_bird);

    // labels = scene_constructor(false);
    // scene_add_literal(labels, &fly);
    // scene_add_literal(labels, &glide);
    // observation = scene_constructor(false);
    // scene_add_literal(observation, &glide);
    // scene_add_literal(observation, &peng);
    // inference = scene_constructor(false);
    // scene_add_literal(inference, &fly);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // old_inactives_size = inactive_rules->length;
    // rule_queue_destructor(&inactive_rules);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 3, false, labels);
    // ck_assert_float_eq_tol(peng_fly->weight, 1, 0.000001);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // ck_assert_int_gt(inactive_rules->length, old_inactives_size);
    // old_inactives_size = inactive_rules->length;
    // rule_queue_destructor(&inactive_rules);

    // scene_destructor(&observation);
    // observation = scene_constructor(false);
    // scene_add_literal(observation, &n_fly);
    // scene_add_literal(observation, &peng);
    // rule_hypergraph_update_rules(knowledge_base, observation, inference, 1, 3, false, labels);
    // ck_assert_float_eq_tol(peng_fly->weight, 2, 0.0000001);
    // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // ck_assert_int_eq(inactive_rules->length, old_inactives_size);
    // rule_queue_destructor(&inactive_rules);

    // // n_fly = literal_constructor_from_string("-fly");
    // // observation = scene_constructor(false);
    // // scene_add_literal(observation, &peng);
    // // // scene_add_literal(observation, &bird);
    // // // scene_add_literal(observation, &fly);
    // // inference = scene_constructor(false);
    // // scene_add_literal(inference, &feathers);
    // // scene_add_literal(inference, &wings);
    // // scene_add_literal(inference, &bird);
    // // scene_add_literal(inference, &fly);

    // // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // // old_inactives_size = inactive_rules->length;
    // // rule_queue_destructor(&inactive_rules);
    // // ck_assert_int_eq(knowledge_base->active->length, 6);
    // // rule_hypergraph_update_rules(knowledge_base, observation, inference, 0.5, 3, false, NULL);
    // // ck_assert_float_eq_tol(bird_peng_fly->weight, 1, 0.000001);
    // // ck_assert_float_eq_tol(peng_fly->weight, 1, 0.000001);
    // // ck_assert_float_eq_tol(wings_bird->weight, 2.5, 0.000001);
    // // ck_assert_float_eq_tol(feathers_bird->weight, 2.5, 0.000001);
    // // ck_assert_float_eq_tol(feathers_wings->weight, 3, 0.000001);
    // // rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    // // ck_assert_int_gt(inactive_rules->length, old_inactives_size);
    // // ck_assert_int_eq(knowledge_base->active->length, 1);

    // literal_destructor(&glide);
    // scene_destructor(&observation);
    // scene_destructor(&inference);
    // scene_destructor(&labels);
    // knowledge_base_destructor(&knowledge_base);
}
END_TEST

Suite *rule_hypergraph_suite() {
    Suite *suite;
    TCase *create_case, *add_edges_case, *adding_and_removing_case, *get_inactive_case,
    *copy_case, *rule_update_case;

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
