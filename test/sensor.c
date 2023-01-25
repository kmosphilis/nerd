#include <check.h>
#include <stdlib.h>

#include "../src/sensor.h"

#define SENSOR_TEST_DATA "./test/data/sensor_test.txt"

START_TEST(construct_destruct_test) {
    Sensor sensor, *sensor_ptr = NULL;

    sensor_constructor_from_file(&sensor, SENSOR_TEST_DATA, 0);
    ck_assert_ptr_nonnull(sensor.environment);
    ck_assert_int_eq(sensor.reuse, 0);
    ck_assert_str_eq(sensor.filepath, SENSOR_TEST_DATA);
    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor.environment);
    ck_assert_int_eq(sensor.reuse, 0);
    ck_assert_pstr_eq(sensor.filepath, NULL);

    sensor_constructor_from_file(&sensor, SENSOR_TEST_DATA, 1);
    ck_assert_ptr_nonnull(sensor.environment);
    ck_assert_int_eq(sensor.reuse, 1);
    ck_assert_str_eq(sensor.filepath, SENSOR_TEST_DATA);
    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor.environment);
    ck_assert_int_eq(sensor.reuse, 0);
    ck_assert_pstr_eq(sensor.filepath, NULL);

    sensor_constructor_from_file(&sensor, "./test/data/filethatdoesntexist.txt", 0);
    ck_assert_ptr_null(sensor.environment);
    ck_assert_int_eq(sensor.reuse, 0);
    ck_assert_pstr_eq(sensor.filepath, NULL);
    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor.environment);
    ck_assert_int_eq(sensor.reuse, 0);
    ck_assert_pstr_eq(sensor.filepath, NULL);

    sensor_constructor_from_file(&sensor, NULL, 0);
    ck_assert_ptr_null(sensor.environment);
    ck_assert_int_eq(sensor.reuse, 0);
    ck_assert_pstr_eq(sensor.filepath, NULL);
    sensor_destructor(&sensor);

    sensor_constructor_from_file(sensor_ptr, SENSOR_TEST_DATA, 0);
    ck_assert_ptr_null(sensor_ptr);
    ck_assert_ptr_null(sensor.environment);
    ck_assert_int_eq(sensor.reuse, 0);
    ck_assert_pstr_eq(sensor.filepath, NULL);
    sensor_destructor(sensor_ptr);
}
END_TEST

START_TEST(get_total_observations_test) {
    Sensor sensor, *sensor_ptr = NULL;
    Scene scene;

    sensor_constructor_from_file(&sensor, SENSOR_TEST_DATA, 1);
    scene_constructor(&scene);

    sensor_get_next_scene(&sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene.size, 5);
    scene_destructor(&scene);

    sensor_get_next_scene(&sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene.size, 5);
    scene_destructor(&scene);

    sensor_get_next_scene(&sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene.size, 5);
    scene_destructor(&scene);

    ck_assert_int_eq(sensor_get_total_observations(&sensor), 4);

    sensor_get_next_scene(&sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene.size, 2);
    scene_destructor(&scene);
    sensor_destructor(&sensor);

    ck_assert_int_eq(sensor_get_total_observations(&sensor), -1);
    ck_assert_int_eq(sensor_get_total_observations(sensor_ptr), -1);
}
END_TEST

START_TEST(get_scene_test) {
    Sensor sensor, *sensor_ptr = NULL;
    Scene scene, original_scene, *scene_ptr = NULL;

    scene_constructor(&scene);
    scene_constructor(&original_scene);
    sensor_constructor_from_file(&sensor, SENSOR_TEST_DATA, 0);

    sensor_get_next_scene(&sensor, &scene, 0, NULL);

    ck_assert_int_eq(scene.size, 5);

    char *string;

    string = literal_to_string(&(scene.observations[0]));
    ck_assert_str_eq(string, "penguin");
    free(string);

    string = literal_to_string(&(scene.observations[1]));
    ck_assert_str_eq(string, "antarctica");
    free(string);

    string = literal_to_string(&(scene.observations[2]));
    ck_assert_str_eq(string, "wings");
    free(string);

    string = literal_to_string(&(scene.observations[3]));
    ck_assert_str_eq(string, "bird");
    free(string);

    string = literal_to_string(&(scene.observations[4]));
    ck_assert_str_eq(string, "-fly");
    free(string);

    scene_destructor(&scene);
    sensor_get_next_scene(&sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene.size, 5);
    string = literal_to_string(&(scene.observations[0]));
    ck_assert_str_eq(string, "albatross");
    free(string);

    string = literal_to_string(&(scene.observations[3]));
    ck_assert_str_eq(string, "wings");
    free(string);

    scene_destructor(&scene);
    sensor_get_next_scene(&sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene.size, 5);
    string = literal_to_string(&(scene.observations[1]));
    ck_assert_str_eq(string, "ocean");
    free(string);

    scene_destructor(&scene);
    sensor_get_next_scene(&sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene.size, 2);

    scene_destructor(&scene);
    sensor_get_next_scene(&sensor, &scene, 0, NULL);
    ck_assert_int_eq(scene.size, 0);
    ck_assert_ptr_null(scene.observations);

    scene_destructor(&scene);
    sensor_get_next_scene(&sensor, NULL, 0, NULL);

    sensor.reuse = 1;
    sensor_get_next_scene(&sensor, &scene, 1, &original_scene);
    ck_assert_int_eq(original_scene.size, 5);
    string = literal_to_string(&(original_scene.observations[0]));
    ck_assert_str_eq(string, "penguin");
    free(string);
    ck_assert_int_le(scene.size, original_scene.size);
    ck_assert_int_ne(scene.size, 0);
    scene_destructor(&scene);
    scene_destructor(&original_scene);

    ck_assert_ptr_null(scene_ptr);
    sensor_get_next_scene(&sensor, &scene, 1, scene_ptr);
    ck_assert_int_le(scene.size, 5);
    ck_assert_ptr_null(scene_ptr);
    scene_destructor(&scene);

    sensor_get_next_scene(&sensor, &scene, 1, NULL);
    ck_assert_int_le(scene.size, 5);
    scene_destructor(&scene);

    sensor_get_next_scene(&sensor, &scene, 1, &original_scene);
    ck_assert_int_eq(original_scene.size, 2);
    ck_assert_int_le(scene.size, original_scene.size);
    ck_assert_int_ne(scene.size, 0);

    scene_destructor(&scene);
    scene_destructor(&original_scene);
    sensor_destructor(&sensor);
    sensor_get_next_scene(sensor_ptr, &scene, 0, NULL);
    ck_assert_ptr_null(sensor_ptr);
    ck_assert_ptr_null(scene.observations);
    ck_assert_int_eq(scene.size, 0);
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
