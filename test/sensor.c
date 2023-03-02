#include <check.h>
#include <stdlib.h>

#include "../src/sensor.h"

#define SENSOR_TEST_DATA "./test/data/sensor_test.txt"

START_TEST(construct_destruct_test) {
    Sensor *sensor = sensor_constructor_from_file(SENSOR_TEST_DATA, 0);

    ck_assert_ptr_nonnull(sensor->environment);
    ck_assert_int_eq(sensor->reuse, 0);
    ck_assert_str_eq(sensor->filepath, SENSOR_TEST_DATA);
    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor);

    sensor = sensor_constructor_from_file(SENSOR_TEST_DATA, 1);
    ck_assert_ptr_nonnull(sensor->environment);
    ck_assert_int_eq(sensor->reuse, 1);
    ck_assert_str_eq(sensor->filepath, SENSOR_TEST_DATA);
    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor);

    sensor = sensor_constructor_from_file("./test/data/filethatdoesntexist.txt", 0);
    ck_assert_ptr_null(sensor);

    sensor = sensor_constructor_from_file(NULL, 0);
    ck_assert_ptr_null(sensor);

    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor);

    sensor_destructor(NULL);
}
END_TEST

START_TEST(get_total_observations_test) {
    Sensor *sensor = sensor_constructor_from_file(SENSOR_TEST_DATA, 1);
    Scene *scene = NULL;

    sensor_get_next_scene(sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene->size, 5);
    scene_destructor(&scene);

    sensor_get_next_scene(sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene->size, 5);
    scene_destructor(&scene);

    sensor_get_next_scene(sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene->size, 5);
    scene_destructor(&scene);

    ck_assert_int_eq(sensor_get_total_observations(sensor), 4);

    sensor_get_next_scene(sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene->size, 2);
    scene_destructor(&scene);
    sensor_destructor(&sensor);

    ck_assert_int_eq(sensor_get_total_observations(sensor), -1);
}
END_TEST

START_TEST(get_scene_test) {
    Sensor *sensor = NULL;
    Scene *scene = NULL, *original_scene = NULL;

    sensor = sensor_constructor_from_file(SENSOR_TEST_DATA, 0);

    sensor_get_next_scene(sensor, &scene, 0, NULL);

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

    sensor_get_next_scene(sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene->size, 5);
    string = literal_to_string(scene->literals[0]);
    ck_assert_str_eq(string, "albatross");
    free(string);

    string = literal_to_string(scene->literals[3]);
    ck_assert_str_eq(string, "wings");
    free(string);
    scene_destructor(&scene);

    sensor_get_next_scene(sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene->size, 5);
    string = literal_to_string(scene->literals[1]);
    ck_assert_str_eq(string, "ocean");
    free(string);
    scene_destructor(&scene);

    sensor_get_next_scene(sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene->size, 2);
    scene_destructor(&scene);

    sensor_get_next_scene(sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene->size, 0);
    ck_assert_ptr_null(scene->literals);
    scene_destructor(&scene);

    sensor_get_next_scene(sensor, NULL, 0, NULL);

    sensor->reuse = 1;
    sensor_get_next_scene(sensor, &scene, 1, &original_scene);
    ck_assert_int_eq(original_scene->size, 5);
    string = literal_to_string(original_scene->literals[0]);
    ck_assert_str_eq(string, "penguin");
    free(string);
    ck_assert_int_le(scene->size, original_scene->size);
    ck_assert_int_ne(scene->size, 0);
    scene_destructor(&scene);
    scene_destructor(&original_scene);

    sensor_get_next_scene(sensor, &scene, 1, NULL);
    ck_assert_int_le(scene->size, 5);
    scene_destructor(&scene);

    sensor_get_next_scene(sensor, &scene, 1, &original_scene);
    ck_assert_int_eq(original_scene->size, 5);
    ck_assert_int_le(scene->size, original_scene->size);
    ck_assert_int_ne(scene->size, 0);

    scene_destructor(&scene);
    scene_destructor(&original_scene);
    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor);
    sensor_get_next_scene(sensor, &scene, 0, NULL);
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
    Suite* suite = sensor_suite();
    SRunner* s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
