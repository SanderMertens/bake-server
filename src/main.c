#include <include/server.h>

/* Return list of packages */
static
bool EndpointProjects(
    ecs_world_t *world,
    ecs_entity_t entity,
    EcsHttpEndpoint *endpoint,
    EcsHttpRequest *request,
    EcsHttpReply *reply)
{
    if (request->method == EcsHttpGet) {
        ut_strbuf result = UT_STRBUF_INIT;
        uint32_t i = 0;

        ut_strbuf_appendstr(&result, "[");

        ut_iter it;
        ut_try( ut_dir_iter(UT_META_PATH, "/*", &it), NULL);
        while (ut_iter_hasNext(&it)) {
            char *id = ut_iter_next(&it);
            char *project_json = ut_asprintf(
                "%s/%s/project.json", UT_META_PATH, id);

            if (ut_file_test(project_json) == 1) {
                char *json = ut_file_load(project_json);
                if (json) {
                    if (i ++) {
                        ut_strbuf_appendstr(&result, ",");
                    }
                    ut_strbuf_appendstr(&result, json);
                    free(json);
                }
            }

            free(project_json);
        }

        ut_strbuf_appendstr(&result, "]");
        reply->body = ut_strbuf_get(&result);
    }
    
    return true;
error:
    reply->status = 501;
    return false;
}

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init_w_args(argc, argv);

    /* Import ECS systems for web server */
    ECS_IMPORT(world, EcsComponentsHttp, 0);
    ECS_IMPORT(world, EcsSystemsCivetweb, 0);

    /* Create server entity */
    ecs_entity_t server = ecs_set(world, 0, EcsHttpServer, {.port = 9091});

    /* Create endpoint entities */
    ecs_entity_t e_projects = ecs_new_child(world, server, NULL, 0);
    ecs_set(world, e_projects, EcsHttpEndpoint, {
        .url = "projects",
        .action = EndpointProjects,
        .synchronous = false 
    });

    /* Main loop ticks once a second, since we have no sychronous endpoints */
    ecs_set_target_fps(world, 1);

    /* Initialize bake util and bake loader */
    ut_init(argv[0]);
    ut_try( ut_load_init(0, 0, 0, 0), NULL);

    /* Run main loop */
    while ( ecs_progress(world, 0));

    /* Cleanup */
    return ecs_fini(world);
error:
    return -1;
}
