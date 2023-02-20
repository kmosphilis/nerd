#include <stdlib.h>
#include <string.h>

#include "../libs/avl-2.0.3/prb.h"
#include "rule_hypergraph.h"

typedef struct Vertex Vertex;

typedef struct Edge {
    const Rule *rule;
    Vertex **from;
    size_t number_of_vertices;
} Edge;

typedef struct Vertex {
    const Literal *literal;
    Edge **edges;
    size_t number_of_edges;
} Vertex;

typedef struct prb_table prb_table;

struct RuleHyperGraph {
    prb_table *literal_tree;
};

/**
 * Destructs the given Edge.
 *
 * @param edge The Edge to be destructed. It should be a reference to the Edge's pointer (a pointer
 * of an Edge pointer).
*/
void edge_destructor(Edge ** const edge) {
    if (edge && *edge) {
        free((*edge)->from);
        free(*edge);
        *edge = NULL;
    }
}

/**
 * Constructs a RuleHyperGraph Vertex.
 *
 * @param literal The Literal which the Vertex will be build for.
 *
 * @return A new Vertex *. Use vertex_destructor to deallocate it.
*/
Vertex *vertex_constructor(const Literal * const literal) {
    Vertex *vertex = (Vertex *) malloc(sizeof(Vertex));
    vertex->literal = literal;
    vertex->edges = NULL;
    vertex->number_of_edges = 0;
}

/**
 * Destructs the given Vertex.
 *
 * @param vertex The Vertex to be destructed. It should be a reference to the Vertex's pointer.
*/
void vertex_destructor(Vertex ** const vertex) {
    if (vertex && *vertex) {
        unsigned int i;
        for (i = 0; i < (*vertex)->number_of_edges; ++i) {
            edge_destructor(&((*vertex)->edges[i]));
        }
        free((*vertex)->edges);
        free(*vertex);
        *vertex = NULL;
    }
}

/**
 * Constructs a RuleHyperGraph Edge.
 *
 * @param rule_hypergraph The RuleHyperGraph to find the corresponding Vertices to be connected.
 * @param rule The Rule to show which Literals should be connected. The Body of a rule has the
 * origin Vertices, and the head has the destination Vertex.
 *
 * @return A new Edge *. Use edge_destructor to deallocate it.
*/
Edge *edge_constructor(RuleHyperGraph * const rule_hypergraph, const Rule * const rule) {
    if (!rule) {
        return NULL;
    }

    Edge *edge = (Edge *) malloc(sizeof(Edge));
    edge->from = (Vertex **) malloc(sizeof(Vertex *) * rule->body.size);
    edge->number_of_vertices = rule->body.size;
    edge->rule = rule;
    Vertex *vertex;
    void *result;

    unsigned int i;
    for (i = 0; i < rule->body.size; ++i) {
        vertex = vertex_constructor(&(rule->body.literals[i]));

        result = prb_insert(rule_hypergraph->literal_tree, vertex);
        if (result) {
            vertex_destructor(&vertex);
            vertex = result;
        }
        edge->from[i] = vertex;
    }
    return edge;
}

/**
 * Adds an Edge to a Vertex. The Vertex is the head of the Rule, and the Edge contains the origin
 * (body) of the Rule.
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
 * Function to be used with the RB-Tree to deallocate the Vertices and the Edges.
*/
void item_destructor(void *item, void *param) {
    Vertex *v = item;
    vertex_destructor(&v);
}

/**
 * Function to be used with the RB-Tree to find the appropriate location of a Vertex through
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
 * Costructs an empty RuleHyperGraph with an empty RB-Tree.
 *
 * @return A new RuleHyperGraph *. Use rule_hypergraph_destructor to deallocate.
*/
RuleHyperGraph *rule_hypergraph_empty_constructor() {
    RuleHyperGraph *hypergraph = malloc(sizeof(RuleHyperGraph));

    hypergraph->literal_tree = prb_create(compare_literals, NULL, NULL);

    return hypergraph;
}

/**
 * Destructs the given RuleHyperGraph and its RB-Tree.
 *
 * @param rule_hypergraph The RuleHyperGraph to be destructed.
*/
void rule_hypergraph_destructor(RuleHyperGraph ** const rule_hypergraph) {
    if (rule_hypergraph && *rule_hypergraph && (*rule_hypergraph)->literal_tree) {
        prb_destroy((*rule_hypergraph)->literal_tree, item_destructor);
        free(*rule_hypergraph);
        *rule_hypergraph = NULL;
    }
}

/**
 * Adds a Rule to the given RuleHyperGraph. This process creates the appropriate Vertices and an
 * Edge to connected them.
 *
 * @param rule_hypergraph The RuleHyperGraph to add the Rule.
 * @param rule The Rule to be added to the RuleHyperGraph.
*/
void rule_hypergraph_add_rule(RuleHyperGraph * const rule_hypergraph, const Rule * const rule) {
    if (rule_hypergraph && rule) {
        Vertex *v = vertex_constructor(&(rule->head));
        void *result = prb_insert(rule_hypergraph->literal_tree, v);

        if (result) {
            vertex_destructor(&v);
            v = result;
        }

        Edge *edge = edge_constructor(rule_hypergraph, rule);
        vertex_add_edge(v, edge);
    }
}

/**
 * Enables testing. Uncomment only for testing.
*/
// #define RULE_HYPERGRAPH_TEST
#ifdef RULE_HYPERGRAPH_TEST

#include <stdio.h>
#include <check.h>

#include "../test/helper/literal.h"
#include "../test/helper/rule.h"

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
    Literal l1, l2;
    literal_constructor(&l1, "penguin", true);
    literal_constructor(&l2, "fly", false);

    Vertex *vertex1 = vertex_constructor(&l1), *vertex2 = vertex_constructor(&l2);

    ck_assert_ptr_eq(vertex1->literal, &l1);
    ck_assert_ptr_null(vertex1->edges);
    ck_assert_int_eq(vertex1->number_of_edges, 0);

    ck_assert_ptr_eq(vertex2->literal, &l2);
    ck_assert_ptr_null(vertex2->edges);
    ck_assert_int_eq(vertex2->number_of_edges, 0);

    vertex_destructor(&vertex1);
    ck_assert_ptr_null(vertex1);
    vertex_destructor(&vertex2);
    ck_assert_ptr_null(vertex2);

    literal_destructor(&l1);
    literal_destructor(&l2);
}
END_TEST

START_TEST(construct_destruct_edge_test) {
    Literal l1, l2, l3, *l_ptr, *l_array = malloc(sizeof(Literal) * 2);
    literal_constructor(&l1, "penguin", true);
    literal_constructor(&l2, "fly", false);
    literal_constructor(&l3, "bird", true);

    Vertex *vertex1 = vertex_constructor(&l1), *vertex2 = vertex_constructor(&l3), *v_array[2];

    Rule r1, r2;
    l_ptr = &l1;
    rule_constructor(&r1, 1, &l_ptr, &l2, 0);
    v_array[0] = vertex1;
    v_array[1] = vertex2;
    l_array[0] = l1;
    l_array[1] = l3;
    rule_constructor(&r2, 2, &l_array, &l2, 0);
    free(l_array);

    RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor();
    prb_insert(hypergraph->literal_tree, vertex1);
    prb_insert(hypergraph->literal_tree, vertex2);

    Edge *edge1 = edge_constructor(hypergraph, &r1), *edge2 = edge_constructor(hypergraph, &r2);

    ck_assert_ptr_eq(edge1->rule, &r1);
    ck_assert_ptr_nonnull(edge1->from);
    ck_assert_int_eq(edge1->number_of_vertices, 1);
    ck_assert_ptr_eq(edge1->from[0], vertex1);
    ck_assert_ptr_eq(edge2->rule, &r2);
    ck_assert_ptr_nonnull(edge2->from);
    ck_assert_int_eq(edge2->number_of_vertices, 2);
    unsigned int i;
    for (i = 0; i < edge2->number_of_vertices; ++i) {
        ck_assert_ptr_eq(edge2->from[i], v_array[i]);
    }

    rule_hypergraph_destructor(&hypergraph);
    edge_destructor(&edge1);
    ck_assert_ptr_null(edge1);
    edge_destructor(&edge2);
    ck_assert_ptr_null(edge2);

    rule_destructor(&r1);
    rule_destructor(&r2);
    literal_destructor(&l1);
    literal_destructor(&l2);
    literal_destructor(&l3);
}
END_TEST

START_TEST(adding_edges_test) {
    RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor();
    Literal l1, l2, l3, *l_ptr, *l_array = malloc(sizeof(Literal) * 2);
    Rule r1, r2, r3;

    literal_constructor(&l1, "penguin", true);
    literal_constructor(&l2, "fly", false);
    literal_constructor(&l3, "bird", true);
    l_ptr = &l1;
    rule_constructor(&r1, 1, &l_ptr, &l2, 0);
    rule_constructor(&r2, 1, &l_ptr, &l3, 0);
    l_array[0] = l1;
    l_array[1] = l3;
    rule_constructor(&r3, 2, &l_array, &l2, 0);
    free(l_array);

    Vertex *v1 = vertex_constructor(&l2), *v2 = vertex_constructor(&l3);
    prb_insert(hypergraph->literal_tree, v1);
    prb_insert(hypergraph->literal_tree, v2);
    Edge *e1 = edge_constructor(hypergraph, &r1), *e2 = edge_constructor(hypergraph, &r2);

    ck_assert_ptr_null(v1->edges);
    ck_assert_int_eq(v1->number_of_edges, 0);

    vertex_add_edge(v1, e1);
    ck_assert_ptr_nonnull(v1->edges);
    ck_assert_int_eq(v1->number_of_edges, 1);
    ck_assert_ptr_eq(v1->edges[0]->rule, &r1);
    ck_assert_int_eq(v1->edges[0]->number_of_vertices, 1);
    ck_assert_literal_eq(v1->edges[0]->from[0]->literal, &l1);

    ck_assert_ptr_null(v2->edges);
    ck_assert_int_eq(v2->number_of_edges, 0);

    vertex_add_edge(v2, e2);
    ck_assert_ptr_nonnull(v2->edges);
    ck_assert_int_eq(v2->number_of_edges, 1);
    ck_assert_ptr_eq(v2->edges[0]->rule, &r2);
    ck_assert_int_eq(v2->edges[0]->number_of_vertices, 1);
    ck_assert_literal_eq(v2->edges[0]->from[0]->literal, &l1);

    e1 = edge_constructor(hypergraph, &r3);

    ck_assert_int_eq(v1->number_of_edges, 1);

    vertex_add_edge(v1, e1);
    ck_assert_int_eq(v1->number_of_edges, 2);
    ck_assert_ptr_eq(v1->edges[0]->rule, &r1);
    ck_assert_int_eq(v1->edges[0]->number_of_vertices, 1);
    ck_assert_literal_eq(v1->edges[0]->from[0]->literal, &l1);
    ck_assert_ptr_eq(v1->edges[1]->rule, &r3);
    ck_assert_int_eq(v1->edges[1]->number_of_vertices, 2);
    ck_assert_literal_eq(v1->edges[1]->from[0]->literal, &l1);
    ck_assert_ptr_eq(v1->edges[1]->from[1]->literal, &l3);

    rule_destructor(&r1);
    rule_destructor(&r2);
    rule_destructor(&r3);
    literal_destructor(&l1);
    literal_destructor(&l2);
    literal_destructor(&l3);
    rule_hypergraph_destructor(&hypergraph);
}
END_TEST

START_TEST(adding_rules_test) {
    RuleHyperGraph *hypergraph = rule_hypergraph_empty_constructor();
    Literal l1, l2, l3, l4, l5, *l_ptr;
    literal_constructor(&l1, "penguin", true);
    literal_constructor(&l2, "fly", false);
    literal_constructor(&l3, "bird", true);
    literal_constructor(&l4, "fly", true);
    literal_constructor(&l5, "wings", true);

    Rule r1, r2, r3, r4, r5, r6;
    l_ptr = &l1;
    rule_constructor(&r1, 1, &l_ptr, &l2, 0);
    l_ptr = &l3;
    rule_constructor(&r2, 1, &l_ptr, &l4, 0);
    rule_constructor(&r3, 1, &l_ptr, &l1, 0);
    l_ptr = &l5;
    rule_constructor(&r4, 1, &l_ptr, &l3, 0);
    Literal *l_array = malloc(sizeof(Literal) * 2);
    l_array[0] = l3;
    l_array[1] = l2;
    rule_constructor(&r5, 2, &l_array, &l1, 0);
    l_array[1] = l1;
    rule_constructor(&r6, 2, &l_array, &l2, 0);
    free(l_array);

    Vertex *v1 = vertex_constructor(&l1), *v2 = vertex_constructor(&l2),
    *v3 = vertex_constructor(&l3), *v4 = vertex_constructor(&l4), *v5 = vertex_constructor(&l5),
    *current_v1, *current_v2;
    Edge *current_e;

    rule_hypergraph_add_rule(hypergraph, &r1);
    current_v1 = prb_find(hypergraph->literal_tree, v2);

    ck_assert_int_eq(current_v1->number_of_edges, 1);
    ck_assert_literal_eq(current_v1->literal, &l2);
    ck_assert_ptr_nonnull(current_v1->edges);
    current_e = current_v1->edges[0];
    ck_assert_ptr_eq(current_e->rule, &r1);
    ck_assert_ptr_nonnull(current_e->from);
    ck_assert_int_eq(current_e->number_of_vertices, 1);
    current_v2 = current_e->from[0];
    ck_assert_literal_eq(current_v2->literal, &l1);
    ck_assert_ptr_null(current_v2->edges);
    ck_assert_int_eq(current_v2->number_of_edges, 0);

    rule_hypergraph_add_rule(hypergraph, &r3);
    ck_assert_ptr_eq(current_v2->edges[0]->rule, &r3);
    ck_assert_literal_eq(current_v2->literal, &l1);
    ck_assert_ptr_nonnull(current_v2->edges);
    ck_assert_int_eq(current_v2->number_of_edges, 1);
    ck_assert_literal_eq(current_v1->literal, &l2);
    ck_assert_literal_eq(current_v1->edges[0]->from[0]->literal, &l1);
    ck_assert_literal_eq(current_v1->edges[0]->from[0]->edges[0]->from[0]->literal, &l3);

    rule_hypergraph_add_rule(hypergraph, &r4);
    current_v1 = prb_find(hypergraph->literal_tree, v3);
    ck_assert_literal_eq(current_v1->literal, &l3);
    ck_assert_int_eq(current_v1->number_of_edges, 1);
    ck_assert_ptr_eq(current_v1->edges[0]->rule, &r4);
    ck_assert_ptr_nonnull(current_v1->edges[0]->from);
    ck_assert_literal_eq(current_v1->edges[0]->from[0]->literal, &l5);

    rule_hypergraph_add_rule(hypergraph, &r2);
    current_v1 = prb_find(hypergraph->literal_tree, v4);
    ck_assert_literal_eq(current_v1->literal, &l4);
    ck_assert_int_eq(current_v1->number_of_edges, 1);
    ck_assert_ptr_eq(current_v1->edges[0]->rule, &r2);
    ck_assert_ptr_eq(current_v2->edges[0]->from[0], current_v1->edges[0]->from[0]);

    rule_hypergraph_add_rule(hypergraph, &r5);
    current_v1 = prb_find(hypergraph->literal_tree, v1);
    ck_assert_literal_eq(current_v1->literal, &l1);
    ck_assert_int_eq(current_v1->number_of_edges, 2);

    current_e = current_v1->edges[0];
    ck_assert_ptr_eq(current_e->rule, &r3);
    ck_assert_int_eq(current_e->number_of_vertices, 1);
    ck_assert_ptr_nonnull(current_e->from);
    ck_assert_literal_eq(current_e->from[0]->literal, &l3);

    current_e = current_v1->edges[1];
    ck_assert_ptr_eq(current_e->rule, &r5);
    ck_assert_int_eq(current_e->number_of_vertices, 2);
    ck_assert_ptr_nonnull(current_e->from);

    ck_assert_literal_eq(current_e->from[0]->literal, &l3);
    ck_assert_ptr_eq(current_e->from[0]->edges[0]->rule, &r4);
    ck_assert_int_eq(current_e->from[0]->number_of_edges, 1);
    ck_assert_literal_eq(current_e->from[0]->edges[0]->from[0]->literal, &l5);
    ck_assert_literal_eq(current_e->from[1]->literal, &l2);
    ck_assert_ptr_eq(current_e->from[1]->edges[0]->rule, &r1);
    ck_assert_int_eq(current_e->from[1]->number_of_edges, 1);
    ck_assert_literal_eq(current_e->from[1]->edges[0]->from[0]->literal, &l1);

    rule_hypergraph_add_rule(hypergraph, &r6);
    current_v1 = prb_find(hypergraph->literal_tree, v2);
    ck_assert_literal_eq(current_v1->literal, &l2);
    ck_assert_int_eq(current_v1->number_of_edges, 2);

    current_e = current_v1->edges[0];
    ck_assert_ptr_eq(current_e->rule, &r1);
    ck_assert_int_eq(current_e->number_of_vertices, 1);
    ck_assert_ptr_nonnull(current_e->from);
    ck_assert_literal_eq(current_e->from[0]->literal, &l1);
    ck_assert_int_eq(current_e->from[0]->number_of_edges, 2);
    ck_assert_ptr_eq(current_e->from[0]->edges[0]->rule, &r3);
    ck_assert_ptr_eq(current_e->from[0]->edges[1]->rule, &r5);

    current_e = current_v1->edges[1];
    ck_assert_ptr_eq(current_e->rule, &r6);
    ck_assert_int_eq(current_e->number_of_vertices, 2);
    ck_assert_ptr_nonnull(current_e->from);
    ck_assert_literal_eq(current_e->from[0]->literal, &l3);
    ck_assert_int_eq(current_e->from[0]->number_of_edges, 1);
    ck_assert_ptr_eq(current_e->from[0]->edges[0]->rule, &r4);
    ck_assert_literal_eq(current_e->from[1]->literal, &l1);
    ck_assert_int_eq(current_e->from[1]->number_of_edges, 2);
    ck_assert_ptr_eq(current_e->from[1]->edges[0]->rule, &r3);
    ck_assert_ptr_eq(current_e->from[1]->edges[1]->rule, &r5);

    vertex_destructor(&v1);
    vertex_destructor(&v2);
    vertex_destructor(&v3);
    vertex_destructor(&v4);
    vertex_destructor(&v5);
    literal_destructor(&l1);
    literal_destructor(&l2);
    literal_destructor(&l3);
    literal_destructor(&l4);
    literal_destructor(&l5);
    rule_destructor(&r1);
    rule_destructor(&r2);
    rule_destructor(&r3);
    rule_destructor(&r4);
    rule_destructor(&r5);
    rule_destructor(&r6);
    rule_hypergraph_destructor(&hypergraph);
}
END_TEST

Suite *rule_hypergraph_suite() {
    Suite *suite;
    TCase *create_case, *add_edges_case, *add_rules_case;

    suite = suite_create("Rule Hypergraph");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_hypergraph_test);
    tcase_add_test(create_case, construct_destruct_vector_test);
    tcase_add_test(create_case, construct_destruct_edge_test);
    suite_add_tcase(suite, create_case);

    add_edges_case = tcase_create("Add Edges");
    tcase_add_test(add_edges_case, adding_edges_test);
    suite_add_tcase(suite, add_edges_case);

    add_rules_case = tcase_create("Add Rules");
    tcase_add_test(add_rules_case, adding_rules_test);
    suite_add_tcase(suite, add_rules_case);

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
