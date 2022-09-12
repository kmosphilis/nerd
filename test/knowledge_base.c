#include <stdlib.h>
#include <check.h>

#include "helper/knowledge_base.h"
#include "helper/rule_queue.h"
#include "helper/rule.h"
#include "../src/knowledge_base.h"

START_TEST(construct_destruct_test) {
    KnowledgeBase knowledge_base, *knowledge_base_ptr = NULL;

    knowledge_base_constructor(&knowledge_base, 5.0);

    ck_assert_float_eq_tol(knowledge_base.activation_threshold, 5.0, 0.00001);  
    ck_assert_rule_queue_empty(&(knowledge_base.active));
    ck_assert_rule_queue_empty(&(knowledge_base.inactive));

    knowledge_base_constructor(knowledge_base_ptr, 5.0);
    ck_assert_ptr_null(knowledge_base_ptr);

    knowledge_base_destructor(&knowledge_base);
    knowledge_base_destructor(knowledge_base_ptr);

    ck_assert_knowledge_base_empty(&knowledge_base);
}
END_TEST

START_TEST(add_rule_test) {
    KnowledgeBase knowledge_base, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue, rule_queue1, rule_queue2;

    create_rule_queue(&rule_queue);
    rule_queue_copy(&rule_queue1, &rule_queue);
    rule_queue_constructor(&rule_queue2);

    unsigned int i, rule_queue_length = rule_queue.length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule rule;
        rule_copy(&rule, &(rule_queue.rules[i]));
        rule_promote(&rule, 3.0);
        rule_queue_enqueue(&rule_queue2, &rule);
        rule_queue_enqueue(&rule_queue, &rule);
        rule_destructor(&rule);
    }

    knowledge_base_constructor(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    rule_queue_destructor(&rule_queue);

    ck_assert_knowledge_base_notempty(&knowledge_base);
    ck_assert_rule_queue_eq(&(knowledge_base.active), &rule_queue2);
    ck_assert_rule_queue_eq(&(knowledge_base.inactive), &rule_queue1);

    knowledge_base_add_rule(knowledge_base_ptr, &rule_queue.rules[0]);
    ck_assert_ptr_null(knowledge_base_ptr);

    rule_queue_destructor(&rule_queue1);
    rule_queue_destructor(&rule_queue2);
    knowledge_base_destructor(&knowledge_base);
}
END_TEST

START_TEST(copy_test) {
    KnowledgeBase knowledge_base1, knowledge_base2, *knowledge_base_ptr1 = NULL,
    *knowledge_base_ptr2 = NULL;
    RuleQueue rule_queue;

    create_rule_queue(&rule_queue);

    unsigned int i, rule_queue_length = rule_queue.length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule rule;
        rule_copy(&rule, &(rule_queue.rules[i]));
        rule_promote(&rule, 3.0);
        rule_queue_enqueue(&rule_queue, &rule);
        rule_destructor(&rule);
    }

    knowledge_base_constructor(&knowledge_base1, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base1, &(rule_queue.rules[i]));
    }

    rule_queue_destructor(&rule_queue);

    knowledge_base_copy(&knowledge_base2, &knowledge_base1);

    ck_assert_knowledge_base_eq(&knowledge_base1, &knowledge_base2);
    ck_assert_ptr_ne(&(knowledge_base1.active), &(knowledge_base2.active));
    ck_assert_ptr_ne(&(knowledge_base1.inactive), &(knowledge_base2.inactive));

    knowledge_base_destructor(&knowledge_base1);

    ck_assert_knowledge_base_empty(&knowledge_base1);
    ck_assert_knowledge_base_notempty(&knowledge_base2);

    knowledge_base_copy(knowledge_base_ptr2, knowledge_base_ptr1);
    knowledge_base_ptr2 = &knowledge_base2;

    knowledge_base_copy(knowledge_base_ptr2, knowledge_base_ptr1);

    ck_assert_knowledge_base_notempty(knowledge_base_ptr2);

    knowledge_base_copy(knowledge_base_ptr1, knowledge_base_ptr2);

    ck_assert_ptr_null(knowledge_base_ptr1);

    knowledge_base_destructor(&knowledge_base2);
}
END_TEST

START_TEST(applicable_rules_test) {
    KnowledgeBase knowledge_base, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue;
    Context context;
    size_t literals_size = 4;

    char *literal_atoms[4] = {"Penguin", "Bird", "Antarctica", "Fly"};
    int literal_signs[4] = {1, 1, 1, 1};
    context_constructor(&context);

    unsigned int i;
    for (i = 0; i < literals_size; ++i) {
        Literal literal;
        literal_constructor(&literal, literal_atoms[i], literal_signs[i]);
        context_add_literal(&context, &literal);
        literal_destructor(&literal);
    }

    create_rule_queue(&rule_queue);

    knowledge_base_constructor(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    rule_queue_destructor(&rule_queue);

    knowledge_base_applicable_rules(&knowledge_base, &context, &rule_queue);

    ck_assert_int_eq(rule_queue.length, 1);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(knowledge_base.inactive.rules[0]));

    rule_queue_destructor(&rule_queue);

    context_destructor(&context);
    char *literal_atoms2[4] = {"Seagull", "Bird", "Antarctica", "Fly"};

    for (i = 0; i < literals_size; ++i) {
        Literal literal;
        literal_constructor(&literal, literal_atoms2[i], literal_signs[i]);
        context_add_literal(&context, &literal);
        literal_destructor(&literal);
    }

    knowledge_base_applicable_rules(&knowledge_base, &context, &rule_queue);

    ck_assert_int_eq(rule_queue.length, 0);

    char *literal_atoms3[4] = {"Seagull", "Bird", "Harbor", "Ocean"};

    for (i = 0; i < literals_size; ++i) {
        Literal literal;
        literal_constructor(&literal, literal_atoms3[i], literal_signs[i]);
        context_add_literal(&context, &literal);
        literal_destructor(&literal);
    }

    knowledge_base_applicable_rules(&knowledge_base, &context, &rule_queue);

    ck_assert_int_eq(rule_queue.length, 1);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(knowledge_base.inactive.rules[2]));

    rule_queue_destructor(&rule_queue);

    context_destructor(&context);
    char *literal_atoms4[4] = {"Albatross", "Bird", "Antarctica", "Penguin"};

    for (i = 0; i < literals_size; ++i) {
        Literal literal;
        literal_constructor(&literal, literal_atoms4[i], literal_signs[i]);
        context_add_literal(&context, &literal);
        literal_destructor(&literal);
    }

    knowledge_base_applicable_rules(&knowledge_base, &context, &rule_queue);

    ck_assert_int_eq(rule_queue.length, 2);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(knowledge_base.inactive.rules[0]));
    ck_assert_rule_eq(&(rule_queue.rules[1]), &(knowledge_base.inactive.rules[1]));
    
    rule_queue_destructor(&rule_queue);

    knowledge_base_applicable_rules(knowledge_base_ptr, &context, &rule_queue);
    ck_assert_ptr_null(knowledge_base_ptr);

    knowledge_base_applicable_rules(&knowledge_base, NULL, &rule_queue);
    ck_assert_rule_queue_empty(&rule_queue);

    knowledge_base_applicable_rules(&knowledge_base, &context, NULL);
    ck_assert_rule_queue_empty(&rule_queue);

    context_destructor(&context);
    knowledge_base_destructor(&knowledge_base);
}
END_TEST

START_TEST(promote_rules_test) {
    KnowledgeBase knowledge_base, knowledge_base2, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue, copy;
    Rule rule;

    unsigned int i;

    create_rule_queue(&rule_queue);

    rule_queue_copy(&copy, &rule_queue);

    knowledge_base_constructor(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    ck_assert_rule_queue_empty(&(knowledge_base.active));
    ck_assert_rule_queue_notempty(&(knowledge_base.inactive));

    ck_assert_int_eq(knowledge_base.active.length, 0);
    ck_assert_int_eq(knowledge_base.inactive.length, 3);

    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 1.0);

    ck_assert_rule_queue_notempty(&(knowledge_base.active));
    ck_assert_int_eq(knowledge_base.active.length, 1);
    ck_assert_int_eq(knowledge_base.inactive.length, 2);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[1]), &copy.rules[1]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 1.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[1].weight, 2.0, 0.00001);

    rule_queue_remove_rule(&rule_queue, 2, &rule);
    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 1.0);
    ck_assert_int_eq(knowledge_base.active.length, 2);
    ck_assert_int_eq(knowledge_base.inactive.length, 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[0]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);

    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 2.0);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[2]), &copy.rules[0]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 5.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 4.0, 0.00001);


    rule_queue_dequeue(&rule_queue, NULL);
    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 1.0);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 6.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 4.0, 0.00001);

    rule_queue_enqueue(&rule_queue, &rule);
    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 3.0);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 6.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 9.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 4.0, 0.00001);

    knowledge_base_promote_rules(knowledge_base_ptr, &rule_queue, 3.0);
    ck_assert_ptr_null(knowledge_base_ptr);

    knowledge_base_copy(&knowledge_base2, &knowledge_base);

    knowledge_base_promote_rules(&knowledge_base, NULL, 1.0);
    ck_assert_knowledge_base_eq(&knowledge_base, &knowledge_base2);

    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 0);
    ck_assert_knowledge_base_eq(&knowledge_base, &knowledge_base2);

    knowledge_base_promote_rules(&knowledge_base, &rule_queue, -1.0);
    ck_assert_knowledge_base_eq(&knowledge_base, &knowledge_base2);

    rule_destructor(&rule);
    rule_queue_destructor(&rule_queue);
    rule_queue_destructor(&copy);
    knowledge_base_destructor(&knowledge_base);
    knowledge_base_destructor(&knowledge_base2);
}
END_TEST

START_TEST(demote_rules_test) {
    KnowledgeBase knowledge_base, knowledge_base2, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue, copy;
    Rule rule;

    unsigned int i;

    create_rule_queue(&rule_queue);

    rule_queue_copy(&copy, &rule_queue);

    knowledge_base_constructor(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        rule_promote(&(rule_queue.rules[i]), 5);
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    ck_assert_rule_queue_notempty(&(knowledge_base.active));
    ck_assert_rule_queue_empty(&(knowledge_base.inactive));

    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 1.0);

    ck_assert_rule_queue_notempty(&(knowledge_base.active));
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[2]), &copy.rules[2]), 1);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 5.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 6.0, 0.00001);

    rule_queue_dequeue(&rule_queue, &rule);
    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 1.0);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[2]), &copy.rules[2]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 5.0, 0.00001);

    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 2.0);
    ck_assert_int_eq(knowledge_base.active.length, 2);
    ck_assert_int_eq(knowledge_base.inactive.length, 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[1]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);


    rule_queue_dequeue(&rule_queue, NULL);
    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 1.0);
    ck_assert_int_eq(knowledge_base.active.length, 1);
    ck_assert_int_eq(knowledge_base.inactive.length, 2);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[1]), &copy.rules[2]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[1].weight, 2.0, 0.00001);

    rule_queue_enqueue(&rule_queue, &rule);
    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 2.0);
    ck_assert_int_eq(knowledge_base.active.length, 0);
    ck_assert_int_eq(knowledge_base.inactive.length, 3);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[1]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[2]), &copy.rules[0]), 1);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[1].weight, 0.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[2].weight, 2.0, 0.00001);

    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 2.0);
    ck_assert_int_eq(knowledge_base.active.length, 0);
    ck_assert_int_eq(knowledge_base.inactive.length, 3);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[1]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[2]), &copy.rules[0]), 1);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[1].weight, 0.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[2].weight, 0.0, 0.00001);


    knowledge_base_demote_rules(knowledge_base_ptr, &rule_queue, 3.0);
    ck_assert_ptr_null(knowledge_base_ptr);

    knowledge_base_copy(&knowledge_base2, &knowledge_base);

    knowledge_base_demote_rules(&knowledge_base, NULL, 1.0);
    ck_assert_knowledge_base_eq(&knowledge_base, &knowledge_base2);

    rule_destructor(&rule);
    rule_queue_destructor(&rule_queue);
    rule_queue_destructor(&copy);
    knowledge_base_destructor(&knowledge_base);
    knowledge_base_destructor(&knowledge_base2);
}
END_TEST

START_TEST(create_new_rules_test) {
    KnowledgeBase knowledge_base, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue;
    Scene observed, inferred;
    Literal literal;

    scene_constructor(&observed);
    scene_constructor(&inferred);

    literal_constructor(&literal, "Penguin", 1);
    scene_add_literal(&observed, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Antarctica", 1);
    scene_add_literal(&observed, &literal);
    scene_add_literal(&inferred, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Bird", 1);
    scene_add_literal(&observed, &literal);
    scene_add_literal(&inferred, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Fly", 0);
    scene_add_literal(&observed, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Albatross", 1);
    scene_add_literal(&observed, &literal);
    literal_destructor(&literal);

    create_rule_queue(&rule_queue);

    unsigned int i, rule_queue_length = rule_queue.length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule rule;
        rule_copy(&rule, &(rule_queue.rules[i]));
        rule_promote(&rule, 3.0);
        rule_queue_enqueue(&rule_queue, &rule);
        rule_destructor(&rule);
    }

    knowledge_base_constructor(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    unsigned int old_size, new_size;
    old_size = knowledge_base.active.length + knowledge_base.inactive.length;
    knowledge_base_create_new_rules(&knowledge_base, &observed, &inferred, 3, 5);
    knowledge_base_create_new_rules(&knowledge_base, &observed, &inferred, 3, 5);
    new_size = knowledge_base.active.length + knowledge_base.inactive.length;
    ck_assert_int_ne(old_size, new_size);
    
    old_size = new_size;
    knowledge_base_create_new_rules(&knowledge_base, &observed, NULL, 3, 5);
    knowledge_base_create_new_rules(&knowledge_base, &observed, NULL, 3, 5);
    new_size = knowledge_base.active.length + knowledge_base.inactive.length;
    ck_assert_int_ne(old_size, new_size);

    old_size = new_size;
    knowledge_base_create_new_rules(&knowledge_base, NULL, &inferred, 3, 5);
    knowledge_base_create_new_rules(&knowledge_base, NULL, &inferred, 3, 5);
    new_size = knowledge_base.active.length + knowledge_base.inactive.length;
    ck_assert_int_eq(old_size, new_size);

    KnowledgeBase copy;
    knowledge_base_copy(&copy, &knowledge_base);

    knowledge_base_create_new_rules(knowledge_base_ptr, &observed, &inferred, 3, 5);
    ck_assert_ptr_null(knowledge_base_ptr);
    ck_assert_knowledge_base_eq(&copy, &knowledge_base);
    knowledge_base_destructor(&copy);

    knowledge_base_destructor(&knowledge_base);
    rule_queue_destructor(&rule_queue);
    scene_destructor(&inferred);
    scene_destructor(&observed);
}
END_TEST

START_TEST(to_string_test) {
    KnowledgeBase knowledge_base, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue;

    create_rule_queue(&rule_queue);

    unsigned int i, rule_queue_length = rule_queue.length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule rule;
        rule_copy(&rule, &(rule_queue.rules[i]));
        rule_promote(&rule, 3.0);
        rule_queue_enqueue(&rule_queue, &rule);
        rule_destructor(&rule);
    }

    knowledge_base_constructor(&knowledge_base, 3.0);
    

    for (i = 0; i < rule_queue_length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    char *knowledge_base_string = knowledge_base_to_string(&knowledge_base);

    ck_assert_str_eq(knowledge_base_string, "Knowledge Base:\n"
    "Activation Threshold: 3.0000\n"
    "Active Rules: [\n]\n"
    "Inactive Rules: [\n"
    "\t(Penguin, Bird, Antarctica) => -Fly (0.0000),\n"
    "\t(Albatross, Bird) => Fly (1.0000),\n"
    "\t(Seagull, Bird, Harbor, Ocean) => Fly (2.0000)\n]");
    free(knowledge_base_string);

    for (; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }
    rule_queue_destructor(&rule_queue);

    knowledge_base_string = knowledge_base_to_string(&knowledge_base);
    ck_assert_str_eq(knowledge_base_string, "Knowledge Base:\n"
        "Activation Threshold: 3.0000\n"
        "Active Rules: [\n"
        "\t(Penguin, Bird, Antarctica) => -Fly (3.0000),\n"
        "\t(Albatross, Bird) => Fly (4.0000),\n"
        "\t(Seagull, Bird, Harbor, Ocean) => Fly (5.0000)\n]\n"
        "Inactive Rules: [\n"
        "\t(Penguin, Bird, Antarctica) => -Fly (0.0000),\n"
        "\t(Albatross, Bird) => Fly (1.0000),\n"
        "\t(Seagull, Bird, Harbor, Ocean) => Fly (2.0000)\n]");
    free(knowledge_base_string);

    rule_queue_destructor(&(knowledge_base.inactive));
    knowledge_base_string = knowledge_base_to_string(&knowledge_base);
    ck_assert_str_eq(knowledge_base_string, "Knowledge Base:\n"
        "Activation Threshold: 3.0000\n"
        "Active Rules: [\n"
        "\t(Penguin, Bird, Antarctica) => -Fly (3.0000),\n"
        "\t(Albatross, Bird) => Fly (4.0000),\n"
        "\t(Seagull, Bird, Harbor, Ocean) => Fly (5.0000)\n]\n"
        "Inactive Rules: [\n]");
    free(knowledge_base_string);

    rule_queue_destructor(&(knowledge_base.active));
    knowledge_base_string = knowledge_base_to_string(&knowledge_base);
    ck_assert_str_eq(knowledge_base_string, "Knowledge Base:\n"
        "Activation Threshold: 3.0000\n"
        "Active Rules: [\n]\n"
        "Inactive Rules: [\n]");
    free(knowledge_base_string);

    knowledge_base_destructor(&knowledge_base);

    knowledge_base_string = knowledge_base_to_string(&knowledge_base);
    ck_assert_str_eq(knowledge_base_string, "Knowledge Base:\n"
        "Activation Threshold: inf\n"
        "Active Rules: [\n]\n"
        "Inactive Rules: [\n]");
    free(knowledge_base_string);

    knowledge_base_string = knowledge_base_to_string(knowledge_base_ptr);
    ck_assert_pstr_eq(knowledge_base_string, NULL);
}
END_TEST

START_TEST(to_prudensjs_test) {
    KnowledgeBase knowledge_base, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue;

    create_rule_queue(&rule_queue);

    unsigned int i, rule_queue_length = rule_queue.length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule rule;
        rule_copy(&rule, &(rule_queue.rules[i]));
        rule_promote(&rule, 3.0);
        rule_queue_enqueue(&rule_queue, &rule);
        rule_destructor(&rule);
    }

    knowledge_base_constructor(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue_length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }
    const char *expected_result = "{\"type\": \"output\", \"kb\": [{\"name\": \"Rule1\", "
    "\"body\": [{\"name\": \"Seagull\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}, {\"name\": \"Bird\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}, {\"name\": \"Harbor\", \"sign\": true, \"isJS\": false, "
    "\"isEquality\": false, \"isInEquality\": false, \"isAction\": false, \"arity\": 0}, "
    "{\"name\": \"Ocean\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}], \"head\": {\"name\": \"Fly\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}}, {\"name\": \"Rule2\", \"body\": ["
    "{\"name\": \"Albatross\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}, {\"name\": \"Bird\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}], \"head\": {\"name\": \"Fly\", \"sign\": true, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}}, {\"name\": \"Rule3\", \"body\": [{\"name\": \"Penguin\", \"sign\": true, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}, {\"name\": \"Bird\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}, {\"name\": \"Antarctica\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}], \"head\": {\"name\": \"Fly\", \"sign\": false, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}}], \"code\": \"\", \"imports\": \"\", \"warnings\": [], "
    "\"customPriorities\": []}";

    char *knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(&knowledge_base);
    ck_assert_pstr_eq(knowledge_base_prudensjs_string, NULL);

    for (; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }
    rule_queue_destructor(&rule_queue);

    knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(&knowledge_base);
    ck_assert_str_eq(knowledge_base_prudensjs_string, expected_result);
    free(knowledge_base_prudensjs_string);

    rule_queue_destructor(&(knowledge_base.inactive));
    knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(&knowledge_base);
    ck_assert_str_eq(knowledge_base_prudensjs_string, expected_result);
    free(knowledge_base_prudensjs_string);

    rule_queue_destructor(&(knowledge_base.active));
    knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(&knowledge_base);
    ck_assert_pstr_eq(knowledge_base_prudensjs_string, NULL);

    knowledge_base_destructor(&knowledge_base);
    knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(&knowledge_base);
    ck_assert_pstr_eq(knowledge_base_prudensjs_string, NULL);

    knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(knowledge_base_ptr);
    ck_assert_pstr_eq(knowledge_base_prudensjs_string, NULL);
}
END_TEST

Suite *knowledge_base_suite() {
    Suite *suite;
    TCase *create_case, *add_rule_case, *applicable_rule_case, *copy_case, *convert_case,
    *rule_weight_manipulation_case, *create_new_rules_case;
    suite = suite_create("Knowledge Base");

    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    add_rule_case = tcase_create("Add Rule");
    tcase_add_test(add_rule_case, add_rule_test);
    suite_add_tcase(suite, add_rule_case);

    applicable_rule_case = tcase_create("Find Applicable Rules");
    tcase_add_test(applicable_rule_case, applicable_rules_test);
    suite_add_tcase(suite, applicable_rule_case);

    rule_weight_manipulation_case = tcase_create("Rule Weight Manipulation");
    tcase_add_test(rule_weight_manipulation_case, promote_rules_test);
    tcase_add_test(rule_weight_manipulation_case, demote_rules_test);
    suite_add_tcase(suite, rule_weight_manipulation_case);

    create_new_rules_case = tcase_create("Create New Rules");
    tcase_add_test(create_new_rules_case, create_new_rules_test);
    suite_add_tcase(suite, create_new_rules_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    convert_case = tcase_create("Conversion");
    tcase_add_test(convert_case, to_string_test);
    tcase_add_test(convert_case, to_prudensjs_test);
    suite_add_tcase(suite, convert_case);

    return suite;
}

int main() {
    Suite* suite = knowledge_base_suite();
    SRunner* s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}