#include <include/server.h>

/* Return JSON configuration of packages */
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
        const char *filter = request->relative_url;
        
        if (!filter || !strlen(filter)) {
            filter = "*";
        }

        ut_strbuf_appendstr(&result, "[");

        ut_iter it;
        ut_try( ut_dir_iter(UT_META_PATH, filter, &it), NULL);
        while (ut_iter_hasNext(&it)) {
            char *id = ut_iter_next(&it);
            char *project_json = ut_asprintf(
                "%s/%s/project.json", UT_META_PATH, id);

            if (ut_file_test(project_json) == 1) {
                char *json = ut_file_load(project_json);
                if (json) {
                    if (i ++) {
                        ut_strbuf_appendstr(&result, ", ");
                    }

                    /* Remove newlines to improve formatting */
                    size_t len = strlen(json);
                    while (json[len - 1] == '\n') {
                        json[len - 1] = '\0';
                        len --;
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

/* Return JSON configuration of packages */
static
bool EndpointInfo(
    ecs_world_t *world,
    ecs_entity_t entity,
    EcsHttpEndpoint *endpoint,
    EcsHttpRequest *request,
    EcsHttpReply *reply)
{
    if (request->method == EcsHttpGet) {
        ut_strbuf result = UT_STRBUF_INIT;
        const char *filter = request->relative_url;
        
        if (!filter || !strlen(filter)) {
            filter = "*";
        }

        ut_strbuf_appendstr(&result, "[");

        int i = 0;
        ut_iter it;
        ut_try( ut_dir_iter(UT_META_PATH, filter, &it), NULL);
        while (ut_iter_hasNext(&it)) {
            char *id = ut_iter_next(&it);
            char *project_json = ut_asprintf(
                "%s/%s/project.json", UT_META_PATH, id);

            if (ut_file_test(project_json) == 1) {
                JSON_Value *jv = json_parse_file(project_json);
                if (!jv) {
                    free(project_json);
                    continue;
                }

                JSON_Object *jo = json_value_get_object(jv);
                if (!jo) {
                    json_value_free(jv);
                    free(project_json);
                    continue;
                }

                const char *type = json_object_get_string(jo, "type");
                const char *author = json_object_dotget_string(jo, "value.author");
                const char *description = json_object_dotget_string(jo, "value.description");
                const char *language = json_object_dotget_string(jo, "value.language");

                if (i ++) {
                    ut_strbuf_appendstr(&result, ", ");
                }

                ut_strbuf_appendstr(&result, "{\n");
                ut_strbuf_append(&result, "  \"id\": \"%s\",\n", id);

                if (!language || strcmp(language, "none")) {
                    ut_strbuf_append(&result, "  \"type\": \"%s\"", type);
                    if (language) {
                        ut_strbuf_append(&result, ",\n  \"language\": \"%s\"", language);
                    } else {
                        ut_strbuf_append(&result, ",\n  \"language\": \"c\"");
                    }
                } else {
                    ut_strbuf_append(&result, "  \"type\": \"config\"");
                }

                if (author) {
                    ut_strbuf_append(&result, ",\n  \"author\":\"%s\"", author);
                }

                if (description) {
                    ut_strbuf_append(&result, ",\n  \"description\":\"%s\"", description);
                }

                ut_strbuf_appendstr(&result, "\n}");

                json_value_free(jv);
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

    /* Create server entity (21115 = 2 1 11 5 = b a k e) */
    ecs_entity_t server = ecs_set(world, 0, EcsHttpServer, {.port = 21115});

    /* Create endpoint entities */
    ecs_entity_t e_projects = ecs_new_child(world, server, NULL, 0);
    ecs_set(world, e_projects, EcsHttpEndpoint, {
        .url = "projects",
        .action = EndpointProjects,
        .synchronous = false 
    });

    ecs_entity_t e_info = ecs_new_child(world, server, NULL, 0);
    ecs_set(world, e_info, EcsHttpEndpoint, {
        .url = "info",
        .action = EndpointInfo,
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
