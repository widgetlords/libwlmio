#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stddef.h>
#include <stdint.h>

#include <threads.h>

#include <wlmio.h>

static mtx_t mutex;


// static void call_soon_threadsafe(PyObject* future, PyObject* result)
// {
//   PyObject* loop = PyObject_CallMethod(future, "get_loop", NULL);
//   PyObject* set_result = PyObject_GetAttrString(future, "set_result");
//   PyObject* rp = PyObject_CallMethod(loop, "call_soon_threadsafe", "(O,O)", set_result, result);
//   Py_XDECREF(rp);
//   Py_DECREF(set_result);
//   Py_DECREF(loop);
// }


static void call_soon(PyObject* const future, PyObject* const result)
{
  PyObject* loop = PyObject_CallMethod(future, "get_loop", NULL);
  PyObject* set_result = PyObject_GetAttrString(future, "set_result");
  PyObject* rp = PyObject_CallMethod(loop, "call_soon", "(O,O)", set_result, result);
  Py_XDECREF(rp);
  Py_DECREF(set_result);
  Py_DECREF(loop);
}


static PyObject* tick(PyObject* const self, PyObject* const args)
{
  PyObject* result = NULL;

  int32_t r = wlmio_tick();
  if(PyErr_Occurred())
  { goto exit; }
  if(r != 0)
  {
    PyErr_SetString(PyExc_SystemError, "Failed to tick");
    goto exit;
  }

  Py_INCREF(Py_None);
  result = Py_None;

exit:
  return result;
}


static PyObject* pywlmio_wait_for_event(PyObject* const self, PyObject* const args)
{
  Py_BEGIN_ALLOW_THREADS
  mtx_lock(&mutex);
  wlmio_wait_for_event();
  mtx_unlock(&mutex);
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject* pywlmio_set_timeout(PyObject* const self, PyObject* const args)
{
  uint64_t timeout;
  int r = PyArg_ParseTuple(args, "K", &timeout);
  if(r == 0)
  { return NULL; }

  wlmio_set_timeout(timeout);

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject* pywlmio_status_callback = NULL;
static PyObject* pywlmio_set_status_callback(PyObject* const self, PyObject* const arg)
{
  PyObject* result = NULL;

  if(!PyCallable_Check(arg)) 
  {
    PyErr_SetString(PyExc_TypeError, "Argument must be callable");
    goto exit;
  }
  Py_XINCREF(arg);
  Py_XDECREF(pywlmio_status_callback);
  pywlmio_status_callback = arg;

  Py_INCREF(Py_None);
  result = Py_None;
  
exit:  
  return result;
}


static void status_callback(const uint8_t node_id, const struct wlmio_status* const old_status, const struct wlmio_status* const new_status)
{
  if(!PyCallable_Check(pywlmio_status_callback))
  { return; }
  
  PyObject* args = Py_BuildValue(
    "B,y#,y#",
    node_id,
    old_status, sizeof(struct wlmio_status),
    new_status, sizeof(struct wlmio_status)
  );

  PyObject* result = PyObject_CallObject(pywlmio_status_callback, args);
  Py_DECREF(args);
  Py_XDECREF(result);
}


struct get_node_info_param
{
  PyObject* future;
  struct wlmio_node_info node_info;
};
static void get_node_info_callback(const int32_t r, void* const p)
{
  const struct get_node_info_param* const param = p;

  PyObject* value = NULL;

  if(r < 0)
  {
    value = Py_None;
    Py_INCREF(value);
  }
  else
  {
    value = Py_BuildValue("y#", &param->node_info, sizeof(struct wlmio_node_info));
  }

  PyObject* result = Py_BuildValue("(i,O)", r, value);
  call_soon(param->future, result);
  Py_DECREF(result);

  Py_DECREF(param->future);
  free((void*)p);
}


static PyObject* get_node_info(PyObject* const self, PyObject* const args)
{
  uint8_t node_id;
  PyObject* future;

  int32_t r = PyArg_ParseTuple(args, "BO", &node_id, &future);
  if(r == 0)
  { goto exit; }
  // if(!PyCallable_Check(callback)) 
  // {
  //   PyErr_SetString(PyExc_TypeError, "Argument must be callable");
  //   goto exit;
  // }

  struct get_node_info_param* const param = malloc(sizeof(struct get_node_info_param));
  param->future = future;
  r = wlmio_get_node_info(node_id, &param->node_info, get_node_info_callback, param);
  if(r < 0) 
  {
    PyErr_SetString(PyExc_SystemError, "get_node_info error");
    free(param);
    goto exit;
  }

  Py_INCREF(future);
  Py_INCREF(future);

exit:
  return future;
}


static PyObject* get_epoll_fd(PyObject* const self, PyObject* const args)
{
  int64_t epollfd = wlmio_get_epoll_fd();
  return PyLong_FromLongLong(epollfd);
}


struct register_access_param
{
  PyObject* future;
  struct wlmio_register_access regr;
};
static void register_access_callback(const int32_t r, void* const p)
{
  const struct register_access_param* const param = p;

  PyObject* value = NULL;

  if(r < 0)
  {
    value = Py_None;
    Py_INCREF(value);
  }
  else
  {
    value = Py_BuildValue("y#", &param->regr, sizeof(struct wlmio_register_access));
  }

  PyObject* result = Py_BuildValue("(i,O)", r, value);
  call_soon(param->future, result);
  Py_DECREF(result);

  Py_DECREF(param->future);
  free((void*)p);
}


static PyObject* register_access(PyObject* const self, PyObject* const args)
{
  PyObject* result = NULL;

  uint8_t node_id;
  PyObject* future;
  const char* name;
  struct wlmio_register_access* regw;
  size_t regw_len;

  int32_t r = PyArg_ParseTuple(args, "Bsy#O", &node_id, &name, &regw, &regw_len, &future);
  if(r == 0)
  {
    // result = Py_None;
    // Py_INCREF(result);
    goto exit;
  }
  // if(!PyCallable_Check(callback)) 
  // {
  //   PyErr_SetString(PyExc_TypeError, "Argument must be callable");
  //   goto exit;
  // }

  struct register_access_param* const param = malloc(sizeof(struct register_access_param));
  param->future = future;
  r = wlmio_register_access(node_id, name, regw, &param->regr, register_access_callback, param);
  if(r < 0) 
  {
    PyErr_SetString(PyExc_SystemError, "register_access error");
    free(param);
    goto exit;
  }

  Py_INCREF(future);
  Py_INCREF(future);
  result = future;

exit:
  return result;
}


struct register_list_param
{
  PyObject* future;
  char name[51];
};
static void register_list_callback(const int32_t r, void* const p)
{
  const struct register_list_param* const param = p;

  PyObject* value = NULL;

  if(r < 0)
  {
    value = Py_None;
    Py_INCREF(value);
  }
  else
  { value = Py_BuildValue("s", &param->name); }

  PyObject* result = Py_BuildValue("(i,O)", r, value);
  call_soon(param->future, result);
  Py_DECREF(result);

  Py_DECREF(param->future);
  free((void*)p);
}


static PyObject* register_list(PyObject* const self, PyObject* const args)
{
  PyObject* result = NULL;

  uint8_t node_id;
  uint16_t index;
  PyObject* future;

  int32_t r = PyArg_ParseTuple(args, "BHO", &node_id, &index, &future);
  if(r == 0)
  { goto exit; }

  struct register_list_param* const param = malloc(sizeof(struct register_list_param));
  param->future = future;
  r = wlmio_register_list(node_id, index, param->name, register_list_callback, param);
  if(r < 0) 
  {
    PyErr_SetString(PyExc_SystemError, "register_access error");
    free(param);
    goto exit;
  }

  Py_INCREF(future);
  Py_INCREF(future);
  result = future;

exit:
  return result;
}


static PyMethodDef wlmio_methods[] = 
{
  {"tick", tick, METH_NOARGS, NULL},
  {"wait_for_event", pywlmio_wait_for_event, METH_NOARGS, NULL},
  {"set_timeout", pywlmio_set_timeout, METH_VARARGS, NULL},
  {"set_status_callback", pywlmio_set_status_callback, METH_O, NULL},
  {"get_node_info", get_node_info, METH_VARARGS, NULL},
  {"get_epoll_fd", get_epoll_fd, METH_NOARGS, NULL},
  {"register_access", register_access, METH_VARARGS, NULL},
  {"register_list", register_list, METH_VARARGS, NULL},
  {NULL, NULL, 0, NULL}
};


static struct PyModuleDef wlmio_module =
{
  PyModuleDef_HEAD_INIT,
  "_wlmio",
  NULL,
  -1,
  wlmio_methods
};


PyMODINIT_FUNC PyInit__wlmio(void)
{
  // asyncio = PyImport_ImportModule("asyncio");
  // if(asyncio == NULL)
  // { return NULL; }

  // future = PyObject_GetAttrString(asyncio, "Future");
  // if(future == NULL)
  // { return NULL; }

  mtx_init(&mutex, mtx_plain);

  int32_t r = wlmio_init();
  if(r != 0)
  {
    PyErr_SetString(PyExc_SystemError, "Failed to initialize libwlmio");
    return NULL;
  }

  wlmio_set_status_callback(status_callback);

  return PyModule_Create(&wlmio_module);
}
