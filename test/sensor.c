#include <check.h>
#include <math.h>
#include <stdlib.h>

#include "../src/sensor.h"

#define SENSOR_TEST_DATA1 "../test/data/sensor_test1.txt"
#define SENSOR_TEST_DATA2 "../test/data/sensor_test2.txt"
#define SENSOR_TEST_DATA3 "../test/data/sensor_test3.txt"

START_TEST(construct_destruct_test) {
  Sensor *sensor = sensor_constructor_from_file(SENSOR_TEST_DATA1, ' ', 0, 0);
  ck_assert_ptr_nonnull(sensor);
  ck_assert_ptr_nonnull(sensor->environment);
  ck_assert_int_eq(sensor->reuse, false);
  ck_assert_str_eq(sensor->filepath, SENSOR_TEST_DATA1);
  ck_assert_int_eq(sensor->delimiter, ' ');
  ck_assert_ptr_eq(sensor->header, NULL);
  ck_assert_int_eq(sensor->header_size, 0);
  sensor_destructor(&sensor);
  ck_assert_ptr_null(sensor);

  sensor = sensor_constructor_from_file(SENSOR_TEST_DATA1, ' ', 1, false);
  ck_assert_ptr_nonnull(sensor->environment);
  ck_assert_int_eq(sensor->reuse, true);
  ck_assert_str_eq(sensor->filepath, SENSOR_TEST_DATA1);
  ck_assert_int_eq(sensor->delimiter, ' ');
  ck_assert_ptr_eq(sensor->header, NULL);
  ck_assert_int_eq(sensor->header_size, 0);
  sensor_destructor(&sensor);
  ck_assert_ptr_null(sensor);

  sensor = sensor_constructor_from_file(SENSOR_TEST_DATA2, ',', true, false);
  ck_assert_ptr_nonnull(sensor->environment);
  ck_assert_int_eq(sensor->reuse, true);
  ck_assert_str_eq(sensor->filepath, SENSOR_TEST_DATA2);
  ck_assert_int_eq(sensor->delimiter, ',');
  ck_assert_ptr_eq(sensor->header, NULL);
  ck_assert_int_eq(sensor->header_size, 0);
  sensor_destructor(&sensor);
  ck_assert_ptr_null(sensor);

  sensor = sensor_constructor_from_file(SENSOR_TEST_DATA3, ',', true, true);
  ck_assert_ptr_nonnull(sensor->environment);
  ck_assert_int_eq(sensor->reuse, true);
  ck_assert_str_eq(sensor->filepath, SENSOR_TEST_DATA3);
  ck_assert_int_eq(sensor->delimiter, ',');
  ck_assert_ptr_ne(sensor->header, NULL);
  ck_assert_int_eq(sensor->header_size, 3);
  unsigned int i;
  const char *header[3] = {"animal", "class", "flies?"};
  for (i = 0; i < sensor->header_size; ++i) {
    ck_assert_str_eq(sensor->header[i], header[i]);
  }
  sensor_destructor(&sensor);
  ck_assert_ptr_null(sensor);

  sensor = sensor_constructor_from_file("./test/data/filethatdoesntexist.txt",
                                        ' ', 0, 1);
  ck_assert_ptr_null(sensor);

  sensor = sensor_constructor_from_file(NULL, ' ', 0, false);
  ck_assert_ptr_null(sensor);

  sensor_destructor(&sensor);
  ck_assert_ptr_null(sensor);

  sensor_destructor(NULL);
}
END_TEST

START_TEST(get_total_observations_test) {
  Sensor *sensor =
      sensor_constructor_from_file(SENSOR_TEST_DATA1, ' ', true, false);
  Scene *scene = NULL;

  ck_assert_int_eq(sensor_get_total_observations(sensor), 4);

  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_eq(scene->size, 5);
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_eq(scene->size, 5);
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_eq(scene->size, 5);
  scene_destructor(&scene);

  ck_assert_int_eq(sensor_get_total_observations(sensor), 4);

  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_eq(scene->size, 2);
  scene_destructor(&scene);
  sensor_destructor(&sensor);

  sensor = sensor_constructor_from_file(SENSOR_TEST_DATA3, ',', true, true);
  ck_assert_int_eq(sensor_get_total_observations(sensor), 4);
  sensor_destructor(&sensor);

  ck_assert_int_eq(sensor_get_total_observations(sensor), -1);
}
END_TEST

START_TEST(get_scene_test) {
  Sensor *sensor = NULL;
  Scene *scene = NULL;

  sensor = sensor_constructor_from_file(SENSOR_TEST_DATA1, ' ', false, false);

  sensor_get_next_scene(sensor, &scene);

  ck_assert_int_eq(scene->size, 5);

  char *string = literal_to_string(scene->literals[0]);
  ck_assert_str_eq(string, "penguin");
  free(string);

  string = literal_to_string(scene->literals[1]);
  ck_assert_str_eq(string, "antarctica");
  free(string);

  string = literal_to_string(scene->literals[2]);
  ck_assert_str_eq(string, "wings");
  free(string);

  string = literal_to_string(scene->literals[3]);
  ck_assert_str_eq(string, "bird");
  free(string);

  string = literal_to_string(scene->literals[4]);
  ck_assert_str_eq(string, "-fly");
  free(string);
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_eq(scene->size, 5);
  string = literal_to_string(scene->literals[0]);
  ck_assert_str_eq(string, "albatross");
  free(string);

  string = literal_to_string(scene->literals[3]);
  ck_assert_str_eq(string, "wings");
  free(string);
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_eq(scene->size, 5);
  string = literal_to_string(scene->literals[1]);
  ck_assert_str_eq(string, "ocean");
  free(string);
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_eq(scene->size, 2);
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  ck_assert_ptr_null(scene);

  sensor_get_next_scene(sensor, NULL);

  sensor->reuse = 1;
  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_le(scene->size, 5);
  scene_destructor(&scene);

  sensor_destructor(&sensor);
  ck_assert_ptr_null(sensor);

  const char *expected_values[2][4] = {
      {"penguin", "bird", "aptenodytes", "forsteri"},
      {"imperial eagle", "aquila", "heliaca"}};

  sensor = sensor_constructor_from_file(SENSOR_TEST_DATA2, ' ', false, false);
  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_le(scene->size, 4);
  unsigned int i;
  for (i = 0; i < (unsigned int)fmin(scene->size, 3); ++i) {
    string = literal_to_string(scene->literals[i]);
    ck_assert_str_ne(expected_values[0][i], string);
    free(string);
  }
  string = literal_to_string(scene->literals[i]);
  ck_assert_str_eq(expected_values[0][i], string);
  free(string);
  scene_destructor(&scene);
  sensor_destructor(&sensor);

  sensor = sensor_constructor_from_file(SENSOR_TEST_DATA2, ',', false, false);
  sensor_get_next_scene(sensor, &scene);
  ck_assert_int_eq(scene->size, 4);
  for (i = 0; i < (unsigned int)fmin(scene->size, 4); ++i) {
    string = literal_to_string(scene->literals[i]);
    ck_assert_str_eq(expected_values[0][i], string);
    free(string);
  }
  scene_destructor(&scene);
  sensor_destructor(&sensor);

  sensor = sensor_constructor_from_file(SENSOR_TEST_DATA3, ',', true, false);

  const char *data3_no_header[5][3] = {{"animal", "class", "flies?"},
                                       {"penguin", "bird", "no"},
                                       {"imperial eagle", "bird", "yes"},
                                       {"bat", "mammal", "yes"},
                                       {"human", "mammal", "no"}};
  const char *data3_with_header[4][3] = {
      {"animal_penguin", "class_bird", "flies?_no"},
      {"animal_imperial eagle", "class_bird", "flies?_yes"},
      {"animal_bat", "class_mammal", "flies?_yes"},
      {"animal_human", "class_mammal", "flies?_no"}};

  sensor_get_next_scene(sensor, &scene);
  for (i = 0; i < scene->size; ++i) {
    string = literal_to_string(scene->literals[i]);
    ck_assert_str_eq(data3_no_header[0][i], string);
    free(string);
  }
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  for (i = 0; i < scene->size; ++i) {
    string = literal_to_string(scene->literals[i]);
    ck_assert_str_eq(data3_no_header[1][i], string);
    free(string);
  }
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  for (i = 0; i < scene->size; ++i) {
    string = literal_to_string(scene->literals[i]);
    ck_assert_str_eq(data3_no_header[2][i], string);
    free(string);
  }
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  for (i = 0; i < scene->size; ++i) {
    string = literal_to_string(scene->literals[i]);
    ck_assert_str_eq(data3_no_header[3][i], string);
    free(string);
  }
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  for (i = 0; i < scene->size; ++i) {
    string = literal_to_string(scene->literals[i]);
    ck_assert_str_eq(data3_no_header[4][i], string);
    free(string);
  }
  scene_destructor(&scene);

  sensor_get_next_scene(sensor, &scene);
  for (i = 0; i < scene->size; ++i) {
    string = literal_to_string(scene->literals[i]);
    ck_assert_str_eq(data3_no_header[0][i], string);
    free(string);
  }
  scene_destructor(&scene);
  sensor_destructor(&sensor);

  sensor = sensor_constructor_from_file(SENSOR_TEST_DATA3, ',', true, true);

  unsigned int j;
  for (j = 0; j < 4; ++j) {
    sensor_get_next_scene(sensor, &scene);
    for (i = 0; i < scene->size; ++i) {
      string = literal_to_string(scene->literals[i]);
      ck_assert_str_eq(data3_with_header[j][i], string);
      // ck_assert_str_eq(data3_no_header[1][i], string);
      free(string);
    }
    scene_destructor(&scene);
  }
  sensor_destructor(&sensor);

  sensor_get_next_scene(sensor, &scene);
  ck_assert_ptr_null(sensor);
  ck_assert_ptr_null(scene);
}
END_TEST

Suite *sensor_suite() {
  Suite *suite;
  TCase *create_case, *get_total_observations_case, *get_scene_case;
  suite = suite_create("Sensor");
  create_case = tcase_create("Create");
  tcase_add_test(create_case, construct_destruct_test);
  suite_add_tcase(suite, create_case);

  get_total_observations_case = tcase_create("Total Observations");
  tcase_add_test(get_total_observations_case, get_total_observations_test);
  suite_add_tcase(suite, get_total_observations_case);

  get_scene_case = tcase_create("Get Scene");
  tcase_add_test(get_scene_case, get_scene_test);
  suite_add_tcase(suite, get_scene_case);

  return suite;
}

int main() {
  Suite *suite = sensor_suite();
  SRunner *s_runner;

  s_runner = srunner_create(suite);
  srunner_set_fork_status(s_runner, CK_NOFORK);

  srunner_run_all(s_runner, CK_ENV);
  int number_failed = srunner_ntests_failed(s_runner);
  srunner_free(s_runner);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
