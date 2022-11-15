#include <check.h>
#include <stdlib.h>

#include "../src/sensor.h"

#define SENSOR_TEST_DATA "./data/sensor_test.txt"

START_TEST(construct_destruct_test) {
    Sensor sensor, *sensor_ptr = NULL;

    sensor_constructor_from_file(&sensor, SENSOR_TEST_DATA, 0);
    ck_assert_ptr_nonnull(sensor.enviroment);
    ck_assert_int_eq(sensor.reuse, 0);
    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor.enviroment);
    ck_assert_int_eq(sensor.reuse, -1);

    sensor_constructor_from_file(&sensor, SENSOR_TEST_DATA, 1);
    ck_assert_ptr_nonnull(sensor.enviroment);
    ck_assert_int_eq(sensor.reuse, 1);
    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor.enviroment);
    ck_assert_int_eq(sensor.reuse, -1);

    sensor_constructor_from_file(&sensor, "./data/filethatdoesntexist.txt", 0);
    ck_assert_ptr_null(sensor.enviroment);
    ck_assert_int_eq(sensor.reuse, -1);
    sensor_destructor(&sensor);
    ck_assert_ptr_null(sensor.enviroment);
    ck_assert_int_eq(sensor.reuse, -1);

    sensor_constructor_from_file(&sensor, NULL, 0);
    ck_assert_ptr_null(sensor.enviroment);
    sensor_destructor(&sensor);

    sensor_constructor_from_file(sensor_ptr, SENSOR_TEST_DATA, 0);
    ck_assert_ptr_null(sensor_ptr);
    ck_assert_ptr_null(sensor.enviroment);
    ck_assert_int_eq(sensor.reuse, -1);
    sensor_destructor(sensor_ptr);
}
END_TEST

START_TEST(get_scene_test) {
    Sensor sensor, *sensor_ptr = NULL;
    Scene scene;

    scene_constructor(&scene);
    sensor_constructor_from_file(&sensor, SENSOR_TEST_DATA, 0);

    sensor_get_next_scene(&sensor, &scene);

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
    sensor_get_next_scene(&sensor, &scene);
    ck_assert_int_eq(scene.size, 5);
    string = literal_to_string(&(scene.observations[0]));
    ck_assert_str_eq(string, "albatross");
    free(string);

    string = literal_to_string(&(scene.observations[3]));
    ck_assert_str_eq(string, "wings");
    free(string);

    scene_destructor(&scene);
    sensor_get_next_scene(&sensor, &scene);
    ck_assert_int_eq(scene.size, 5);
    string = literal_to_string(&(scene.observations[1]));
    ck_assert_str_eq(string, "ocean");
    free(string);

    scene_destructor(&scene);
    sensor_get_next_scene(&sensor, &scene);
    ck_assert_int_eq(scene.size, 2);

    scene_destructor(&scene);
    sensor_get_next_scene(&sensor, &scene);
    ck_assert_int_eq(scene.size, 0);
    ck_assert_ptr_null(scene.observations);

    scene_destructor(&scene);
    sensor_get_next_scene(&sensor, NULL);

    sensor.reuse = 1;
    sensor_get_next_scene(&sensor, &scene);
    ck_assert_int_eq(scene.size, 5);
    string = literal_to_string(&(scene.observations[0]));
    ck_assert_str_eq(string, "penguin");
    free(string);

    scene_destructor(&scene);
    sensor_destructor(&sensor);
    sensor_get_next_scene(sensor_ptr, &scene);
    ck_assert_ptr_null(sensor_ptr);
    ck_assert_ptr_null(scene.observations);
    ck_assert_int_eq(scene.size, 0);
}
END_TEST

Suite *sensor_suite() {
    Suite *suite;
    TCase *create_case, *get_scene_case;
    suite = suite_create("Sensor");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

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