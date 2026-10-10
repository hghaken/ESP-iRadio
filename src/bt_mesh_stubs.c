// Empty BLE Mesh dispatch handlers (flash saving, ~120 KB).
//
// The prebuilt Arduino framework is configured with CONFIG_BLE_MESH=y, so Bluedroid's
// BTC dispatch table (btc_task.c) references these handlers and the linker pulls in
// the whole Mesh stack behind them. The radio never initialises BLE Mesh, so no Mesh
// message can ever be dispatched; defining the handlers here as no-ops keeps the
// linker from taking the library objects. If a future framework stops referencing a
// handler, this file only costs a few bytes. If the link ever reports a duplicate
// definition, a library object is still needed for another symbol: delete this file.
//
// Signatures match esp_ble_mesh/btc/include/*.h: void handler(btc_msg_t *msg).

void btc_ble_mesh_config_server_cb_handler(void* msg) { (void)msg; }
void btc_ble_mesh_generic_server_cb_handler(void* msg) { (void)msg; }
void btc_ble_mesh_health_server_call_handler(void* msg) { (void)msg; }
void btc_ble_mesh_health_server_cb_handler(void* msg) { (void)msg; }
void btc_ble_mesh_lighting_server_cb_handler(void* msg) { (void)msg; }
void btc_ble_mesh_model_call_handler(void* msg) { (void)msg; }
void btc_ble_mesh_model_cb_handler(void* msg) { (void)msg; }
void btc_ble_mesh_prov_call_handler(void* msg) { (void)msg; }
void btc_ble_mesh_prov_cb_handler(void* msg) { (void)msg; }
void btc_ble_mesh_sensor_server_cb_handler(void* msg) { (void)msg; }
void btc_ble_mesh_time_scene_server_cb_handler(void* msg) { (void)msg; }
