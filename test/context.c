#include <check.h>
#include <stdlib.h>

#include "../src/context.h"

START_TEST(to_prudensjs_test) {
  Context *context = context_constructor(true);
  Literal *l1 = literal_constructor("Penguin", 1),
          *l2 = literal_constructor("Antarctica", 1),
          *l3 = literal_constructor("Bird", 1),
          *l4 = literal_constructor("Fly", 0);

  context_add_literal(context, &l1);
  context_add_literal(context, &l2);
  context_add_literal(context, &l3);
  context_add_literal(context, &l4);

  char *context_prudensjs_string = context_to_prudensjs(context);
  ck_assert_str_eq(
      context_prudensjs_string,
      "{\"type\": \"output\", \"context\": ["
      "{\"name\": \"penguin\", \"sign\": true, \"isJS\": false, "
      "\"isEquality\": "
      "false, \"isInEquality\": false, \"isAction\": false, \"arity\": "
      "0}, {\"name\": \"antarctica\", \"sign\": true, \"isJS\": false, "
      "\"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
      "\"arity\": 0}, {\"name\": \"bird\", \"sign\": true, "
      "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
      "\"isAction\": false, \"arity\": 0}, {\"name\": \"fly\", "
      "\"sign\": false, \"isJS\": false, \"isEquality\": false, "
      "\"isInEquality\": "
      "false, \"isAction\": false, \"arity\": 0}]}");
  free(context_prudensjs_string);

  scene_remove_literal(context, 1, NULL);
  scene_remove_literal(context, 2, NULL);
  context_prudensjs_string = context_to_prudensjs(context);
  ck_assert_str_eq(
      context_prudensjs_string,
      "{\"type\": \"output\", \"context\": ["
      "{\"name\": \"penguin\", \"sign\": true, \"isJS\": false, "
      "\"isEquality\": "
      "false, \"isInEquality\": false, \"isAction\": false, \"arity\": "
      "0}, {\"name\": \"bird\", \"sign\": true, \"isJS\": false, "
      "\"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
      "\"arity\": 0}]}");
  free(context_prudensjs_string);
  context_destructor(&context);

  context_prudensjs_string = context_to_prudensjs(context);
  ck_assert_pstr_eq(context_prudensjs_string, NULL);
}
END_TEST

Suite *context_suite() {
  Suite *suite;
  TCase *to_prudensjs_case;
  suite = suite_create("Context");

  to_prudensjs_case = tcase_create("To Prudens JS");
  tcase_add_test(to_prudensjs_case, to_prudensjs_test);
  suite_add_tcase(suite, to_prudensjs_case);

  return suite;
}

int main() {
  Suite *suite = context_suite();
  SRunner *s_runner = srunner_create(suite);

  srunner_set_fork_status(s_runner, CK_NOFORK);
  srunner_run_all(s_runner, CK_ENV);
  int number_of_failed = srunner_ntests_failed(s_runner);
  srunner_free(s_runner);

  return (number_of_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
