#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../libs/avl-2.0.3/prb.h"
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
        free((*edge)->from);
        rule_destructor(&((*edge)->rule));
        free(*edge);
        *edge = NULL;
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
        free((*vertex)->edges);
        if (destruct_literal) {
            literal_destructor(&((*vertex)->literal));
        }
        free(*vertex);
        *vertex = NULL;
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
        free((*rule)->body);
        free((*rule));
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
            free(vertex->edges);
            vertex->edges = NULL;
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
    Vertex *v = item;
    vertex_destructor(&v, true);
}

/**
 * @brief Function to be used with the RB-Tree to find the appropriate location of a Vertex through
 * comparison.
*/
int compare_literals(const void *vertex1, const void *vertex2, void *extra) {
    const Vertex *v1 = vertex1, *v2 = vertex2;
    char *l1_string = literal_to_string(v1->literal), *l2_string = literal_to_string(v2->literal);
    int result = strcmp(l1_string, l2_string);

    free(l1_string);
    free(l2_string);

    return result;
}

/**
 * @brief Costructs an empty RuleHyperGraph with an empty RB-Tree.
 *
 * @return A new RuleHyperGraph *. Use rule_hypergraph_destructor to deallocate.
*/
RuleHyperGraph *rule_hypergraph_empty_constructor() {
    RuleHyperGraph *hypergraph = (RuleHyperGraph *) malloc(sizeof(RuleHyperGraph));

    hypergraph->literal_tree = prb_create(compare_literals, NULL, NULL);

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
        free(*rule_hypergraph);
        *rule_hypergraph = NULL;
    }
}

/**
 * @brief Adds a Rule to the given RuleHyperGraph. This process creates the appropriate Vertices and
 * an Edge to connected them.
 *
 * @param rule_hypergraph The RuleHyperGraph to add the Rule.
 * @param rule The Rule to be added to the RuleHyperGraph. Rules should not have the ownership of
 * the given literals.
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
 * given double pointer is not already allocated, otherwise its contents will be lost in the merory.
 * The RuleQueue which will be created will not have ownership of its Rules.
*/
void rule_hypergraph_get_inactive_rules(KnowledgeBase * const knowledge_base,
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
 * Enables testing. Uncomment only for testing.
*/
// #define RULE_HYPERGRAPH_TEST
#ifdef RULE_HYPERGRAPH_TEST

#include <check.h>

#include "../test/helper/literal.h"
#include "../test/helper/rule.h"
#include "../test/helper/rule_queue.h"

START_TEST(construct_destruct_hypergraph_test) {
    RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor();

    ck_assert_ptr_nonnull(hypergraph);
    ck_assert_int_eq(hypergraph->literal_tree->prb_count, 0);
    ck_assert_ptr_null(hypergraph->literal_tree->prb_param);
    ck_assert_ptr_null(hypergraph->literal_tree->prb_root);
    ck_assert_ptr_eq(hypergraph->literal_tree->prb_compare, compare_literals);

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
    RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor();

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
    RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor();
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true), *l_array[2] = {l1, l3};
    Vertex *v1 = vertex_constructor(l2), *v2 = vertex_constructor(l3);
    Rule *r1 = rule_constructor(1, &l1, &l2, 0, false), *r1_ptr = r1,
    *r2 = rule_constructor(1, &l1, &l3, 0, false), *r2_ptr = r2,
    *r3 = rule_constructor(2, l_array, &l2, 0, false), *r3_ptr = r3;

    prb_insert(hypergraph->literal_tree, v1);
    prb_insert(hypergraph->literal_tree, v2);

    Edge *e1 = edge_constructor(hypergraph, &r1, v1), *e2 = edge_constructor(hypergraph, &r2, v2);

    ck_assert_ptr_null(v1->edges);
    ck_assert_int_eq(v1->number_of_edges, 0);

    vertex_add_edge(v1, e1);
    ck_assert_ptr_nonnull(v1->edges);
    ck_assert_int_eq(v1->number_of_edges, 1);
    ck_assert_ptr_eq(v1->edges[0]->rule, r1_ptr);
    ck_assert_int_eq(v1->edges[0]->number_of_vertices, 1);
    ck_assert_literal_eq(v1->edges[0]->from[0]->literal, l1);

    ck_assert_ptr_null(v2->edges);
    ck_assert_int_eq(v2->number_of_edges, 0);

    vertex_add_edge(v2, e2);
    ck_assert_ptr_nonnull(v2->edges);
    ck_assert_int_eq(v2->number_of_edges, 1);
    ck_assert_ptr_eq(v2->edges[0]->rule, r2_ptr);
    ck_assert_int_eq(v2->edges[0]->number_of_vertices, 1);
    ck_assert_literal_eq(v2->edges[0]->from[0]->literal, l1);

    e1 = edge_constructor(hypergraph, &r3, v1);

    ck_assert_int_eq(v1->number_of_edges, 1);

    vertex_add_edge(v1, e1);
    ck_assert_int_eq(v1->number_of_edges, 2);
    ck_assert_ptr_eq(v1->edges[0]->rule, r1_ptr);
    ck_assert_int_eq(v1->edges[0]->number_of_vertices, 1);
    ck_assert_literal_eq(v1->edges[0]->from[0]->literal, l1);
    ck_assert_ptr_eq(v1->edges[1]->rule, r3_ptr);
    ck_assert_int_eq(v1->edges[1]->number_of_vertices, 2);
    ck_assert_literal_eq(v1->edges[1]->from[0]->literal, l1);
    ck_assert_literal_eq(v1->edges[1]->from[1]->literal, l3);

    rule_hypergraph_destructor(&hypergraph);
}
END_TEST

START_TEST(adding_rules_test) {
    RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor();
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
    ck_assert_int_eq(rule_hypergraph_add_rule(hypergraph, &r1), 1);
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
    RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor();
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true), *l4 = literal_constructor("fly", true),
    *l5 = literal_constructor("wings", true), *l_array[2] = {l3, l5};
    Vertex *v1 = vertex_constructor(l1), *v2 = vertex_constructor(l2),
    *v3 = vertex_constructor(l3), *v4 = vertex_constructor(l4), *v5 = vertex_constructor(l5),
    *current_v1, *current_v2;
    Rule *r1 = rule_constructor(1, &l1, &l2, 0, false), *r1_ptr = r1,
    *r2 = rule_constructor(1, &l3, &l4, 0, false), *r2_ptr = r2,
    *r3 = rule_constructor(1, &l5, &l4, 0, false), *r3_ptr = r3,
    *r4 = rule_constructor(1, &l5, &l3, 0, false), *r4_ptr = r4,
    *r5 = rule_constructor(2, l_array, &l4, 0, false), *r5_ptr = r5;
    l_array[1] = l1;
    Rule *r6 = rule_constructor(2, l_array, &l2, 0, false), *r6_ptr = r6, *copy;

    rule_hypergraph_add_rule(hypergraph, &r1);
    rule_hypergraph_add_rule(hypergraph, &r2);
    rule_hypergraph_add_rule(hypergraph, &r3);
    rule_hypergraph_add_rule(hypergraph, &r4);
    rule_hypergraph_add_rule(hypergraph, &r5);
    rule_hypergraph_add_rule(hypergraph, &r6);

    current_v1 = prb_find(hypergraph->literal_tree, v3);
    ck_assert_literal_eq(current_v1->literal, l3);
    ck_assert_int_eq(current_v1->number_of_edges, 1);
    ck_assert_ptr_eq(current_v1->edges[0]->rule, r4_ptr);

    rule_copy(&copy, r2_ptr);
    rule_hypergraph_remove_rule(hypergraph, r4_ptr);
    ck_assert_literal_eq(current_v1->literal, l3);
    ck_assert_int_eq(current_v1->number_of_edges, 0);
    ck_assert_ptr_null(current_v1->edges);

    rule_hypergraph_remove_rule(hypergraph, copy);
    rule_destructor(&copy);

    current_v1 = prb_find(hypergraph->literal_tree, v4);
    ck_assert_int_eq(current_v1->number_of_edges, 3);
    ck_assert_ptr_eq(current_v1->edges[0]->rule, r2_ptr);
    ck_assert_ptr_eq(current_v1->edges[1]->rule, r3_ptr);
    ck_assert_ptr_eq(current_v1->edges[2]->rule, r5_ptr);

    rule_hypergraph_remove_rule(hypergraph, r3_ptr);
    ck_assert_int_eq(current_v1->number_of_edges, 2);
    ck_assert_ptr_eq(current_v1->edges[0]->rule, r2_ptr);
    ck_assert_ptr_eq(current_v1->edges[1]->rule, r5_ptr);

    rule_hypergraph_remove_rule(hypergraph, r2_ptr);
    ck_assert_int_eq(current_v1->number_of_edges, 1);
    ck_assert_ptr_eq(current_v1->edges[0]->rule, r5_ptr);

    current_v2 = prb_find(hypergraph->literal_tree, v2);
    ck_assert_int_eq(current_v2->number_of_edges, 2);
    ck_assert_ptr_eq(current_v2->edges[0]->rule, r1_ptr);
    ck_assert_ptr_eq(current_v2->edges[1]->rule, r6_ptr);

    rule_hypergraph_remove_rule(hypergraph, r1_ptr);
    ck_assert_int_eq(current_v2->number_of_edges, 1);
    ck_assert_ptr_eq(current_v2->edges[0]->rule, r6_ptr);
    ck_assert_int_eq(current_v1->number_of_edges, 1);
    ck_assert_ptr_eq(current_v1->edges[0]->rule, r5_ptr);

    rule_hypergraph_remove_rule(hypergraph, r5_ptr);
    ck_assert_int_eq(current_v1->number_of_edges, 0);
    ck_assert_ptr_null(current_v1->edges);
    ck_assert_int_eq(current_v2->number_of_edges, 1);
    ck_assert_ptr_eq(current_v2->edges[0]->rule, r6_ptr);

    rule_hypergraph_remove_rule(hypergraph, r6_ptr);
    ck_assert_int_eq(current_v2->number_of_edges, 0);
    ck_assert_ptr_null(current_v2->edges);
    ck_assert_int_eq(current_v1->number_of_edges, 0);
    ck_assert_ptr_null(current_v1->edges);

    vertex_destructor(&v1, false);
    vertex_destructor(&v2, false);
    vertex_destructor(&v3, false);
    vertex_destructor(&v4, false);
    vertex_destructor(&v5, false);
    rule_hypergraph_destructor(&hypergraph);
}
END_TEST

START_TEST(get_inactive_rules_test) {
    KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0);
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

Suite *rule_hypergraph_suite() {
    Suite *suite;
    TCase *create_case, *add_edges_case, *adding_and_removing_case, *get_inactive_case;

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
