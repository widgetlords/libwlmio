#include <stddef.h>
#include <stdint.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <canard.h>

#include "wlmio.h"


struct vpe6010_read
{
  struct wlmio_register_access reg;
  struct wlmio_vpe6010_input* input;
  void (* callback)(int32_t r, void* uparam);
  void* uparam;
};


static void vpe6010_read_callback(int32_t r, void* const p)
{
  struct vpe6010_read* const t = p;

  if(r < 0)
  { goto exit; }

  if(t->reg.type != WLMIO_REGISTER_VALUE_UINT16 || t->reg.length < 6)
  {
    r = -EPROTO;
    goto exit;
  }

  memcpy(&t->input->ma_5v, t->reg.value + 0, 2);
  memcpy(&t->input->mv_5v, t->reg.value + 2, 2);
  memcpy(&t->input->mv_24v1, t->reg.value + 4, 2);
  memcpy(&t->input->mv_24v2, t->reg.value + 6, 2);
  memcpy(&t->input->mv_24v, t->reg.value + 8, 2);
  memcpy(&t->input->ma_24v, t->reg.value + 10, 2);

  r = 0;

exit:
  t->callback(r, t->uparam);
  free(p);
}


int32_t wlmio_vpe6010_read(const uint8_t node_id, struct wlmio_vpe6010_input* const dst, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
	if(node_id > CANARD_NODE_ID_MAX || dst == NULL) { return -EINVAL; }

  struct vpe6010_read* t = malloc(sizeof(struct vpe6010_read));
  t->input = dst;
  t->callback = callback;
  t->uparam = uparam;

	int32_t r = wlmio_register_access(node_id, "input", NULL, &t->reg, &vpe6010_read_callback, t);
  if(r < 0)
  {
    free(t);
    goto exit;
  }

  r = 0;

exit:
	return r;
}


int32_t wlmio_vpe6030_write(const uint8_t node_id, const uint8_t ch, uint8_t v, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
	if(node_id > CANARD_NODE_ID_MAX || ch > 3) { return -EINVAL; }

	struct wlmio_register_access regw;

	regw.type = WLMIO_REGISTER_VALUE_UINT8;
	regw.length = 1;

  v = v ? 1U : 0U;

	memcpy(regw.value, &v, 1);

  const char* name = NULL;
  switch(ch)
  {
    case 0:
      name = "ch1.output";
      break;

    case 1:
      name = "ch2.output";
      break;

    case 2:
      name = "ch3.output";
      break;

    case 3:
      name = "ch4.output";
      break;
  }

	int32_t r = wlmio_register_access(node_id, name, &regw, NULL, callback, uparam);

	return r;
}


struct vpe6040_read
{
  struct wlmio_register_access reg;
  uint16_t* input;
  void (* callback)(int32_t r, void* uparam);
  void* uparam;
};


static void vpe6040_read_callback(int32_t r, void* const p)
{
  struct vpe6040_read* const t = p;

  if(r < 0)
  { goto exit; }

  if(t->reg.type != WLMIO_REGISTER_VALUE_UINT16 || t->reg.length < 1)
  {
    r = -EPROTO;
    goto exit;
  }

  memcpy(t->input, t->reg.value, 2);

  r = 0;

exit:
  t->callback(r, t->uparam);
  free(p);
}


int32_t wlmio_vpe6040_read(const uint8_t node_id, const uint8_t ch, uint16_t* const v, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 3 || v == NULL || callback == NULL) { return -EINVAL; }

  struct vpe6040_read* t = malloc(sizeof(struct vpe6040_read));
  t->input = v;
  t->callback = callback;
  t->uparam = uparam;

  const char* name = NULL;
  switch(ch)
  {
     case 0:
      name = "ch1.input";
      break;

    case 1:
      name = "ch2.input";
      break;

    case 2:
      name = "ch3.input";
      break;

    case 3:
      name = "ch4.input";
      break;
  }
	int32_t r = wlmio_register_access(node_id, name, NULL, &t->reg, &vpe6040_read_callback, t);
  if(r < 0)
  {
    free(t);
    goto exit;
  }

  r = 0;

exit:
	return r;
}


int32_t wlmio_vpe6040_configure(const uint8_t node_id, const uint8_t ch, const uint8_t mode, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 3 || mode > WLMIO_VPE6040_MODE_10V || callback == NULL) { return -EINVAL; }

  struct wlmio_register_access regw;

	regw.type = WLMIO_REGISTER_VALUE_UINT8;
	regw.length = 1;

	memcpy(regw.value, &mode, 1);

  const char* name = NULL;
  switch(ch)
  {
    case 0:
      name = "ch1.mode";
      break;

    case 1:
      name = "ch2.mode";
      break;

    case 2:
      name = "ch3.mode";
      break;

    case 3:
      name = "ch4.mode";
      break;
  }

	int32_t r = wlmio_register_access(node_id, name, &regw, NULL, callback, uparam);

	return r;
}


int32_t wlmio_vpe6050_write(const uint8_t node_id, const uint8_t ch, const uint16_t v, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 3 || callback == NULL) { return -EINVAL; }

  struct wlmio_register_access regw;

	regw.type = WLMIO_REGISTER_VALUE_UINT16;
	regw.length = 1;

	memcpy(regw.value, &v, 2);

  const char* name = NULL;
  switch(ch)
  {
    case 0:
      name = "ch1.output";
      break;

    case 1:
      name = "ch2.output";
      break;

    case 2:
      name = "ch3.output";
      break;

    case 3:
      name = "ch4.output";
      break;
  }

	int32_t r = wlmio_register_access(node_id, name, &regw, NULL, callback, uparam);

	return r;
}


int32_t wlmio_vpe6050_configure(const uint8_t node_id, const uint8_t ch, const uint8_t mode, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 3 || mode > WLMIO_VPE6050_MODE_SINK || callback == NULL) { return -EINVAL; }

  struct wlmio_register_access regw;

	regw.type = WLMIO_REGISTER_VALUE_UINT8;
	regw.length = 1;

	memcpy(regw.value, &mode, 1);

  const char* name = NULL;
  switch(ch)
  {
    case 0:
      name = "ch1.mode";
      break;

    case 1:
      name = "ch2.mode";
      break;

    case 2:
      name = "ch3.mode";
      break;

    case 3:
      name = "ch4.mode";
      break;
  }

	int32_t r = wlmio_register_access(node_id, name, &regw, NULL, callback, uparam);

	return r;
}


struct vpe6060_read
{
  struct wlmio_register_access reg;
  uint32_t* input;
  void (* callback)(int32_t r, void* uparam);
  void* uparam;
};


static void vpe6060_read_callback(int32_t r, void* const p)
{
  struct vpe6060_read* const t = p;

  if(r < 0)
  { goto exit; }

  if(t->reg.type != WLMIO_REGISTER_VALUE_UINT32 || t->reg.length < 1)
  {
    r = -EPROTO;
    goto exit;
  }

  memcpy(t->input, t->reg.value, 2);

  r = 0;

exit:
  t->callback(r, t->uparam);
  free(p);
}


int32_t wlmio_vpe6060_read(const uint8_t node_id, const uint8_t ch, uint32_t* const v, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 3 || v == NULL || callback == NULL) { return -EINVAL; }

  struct vpe6060_read* t = malloc(sizeof(struct vpe6060_read));
  t->input = v;
  t->callback = callback;
  t->uparam = uparam;

  const char* name = NULL;
  switch(ch)
  {
     case 0:
      name = "ch1.input";
      break;

    case 1:
      name = "ch2.input";
      break;

    case 2:
      name = "ch3.input";
      break;

    case 3:
      name = "ch4.input";
      break;
  }
	int32_t r = wlmio_register_access(node_id, name, NULL, &t->reg, &vpe6060_read_callback, t);
  if(r < 0)
  {
    free(t);
    goto exit;
  }

  r = 0;

exit:
	return r;
}


struct vpe6060_configure
{
  uint8_t counter;
  uint8_t cancelled;
  int32_t r;
  void (* callback)(int32_t r, void* uparam);
  void* uparam;
};


static void vpe6060_configure_callback(int32_t r, void* const p)
{
  struct vpe6060_configure* const t = p;

  t->counter += 1;

  if(r < 0 && t->r >= 0)
  { t->r = r; }

  if(t->counter + t->cancelled < 3)
  { return; }

  t->callback(t->r, t->uparam);
  free(p);
}


int32_t wlmio_vpe6060_configure(const uint8_t node_id, const uint8_t ch, const uint8_t mode, const uint8_t polarity, const uint8_t bias, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 3 || mode > WLMIO_VPE6060_MODE_PULSE_COUNTER || polarity > WLMIO_VPE6060_POLARITY_FALLING || bias > WLMIO_VPE6060_BIAS_NPN || callback == NULL) { return -EINVAL; }

  struct vpe6060_configure* t = malloc(sizeof(struct vpe6060_configure));
  t->counter = 0;
  t->cancelled = 0;
  t->r = 0;
  t->callback = callback;
  t->uparam = uparam;

  struct wlmio_register_access regw;

	regw.type = WLMIO_REGISTER_VALUE_UINT8;
	regw.length = 1;

  const char* mode_name = NULL;
  const char* polarity_name = NULL;
  const char* bias_name = NULL;
  switch(ch)
  {
    case 0:
      mode_name = "ch1.mode";
      polarity_name = "ch1.polarity";
      bias_name = "ch1.bias";
      break;

    case 1:
      mode_name = "ch2.mode";
      polarity_name = "ch2.polarity";
      bias_name = "ch2.bias";
      break;

    case 2:
      mode_name = "ch3.mode";
      polarity_name = "ch3.polarity";
      bias_name = "ch3.bias";
      break;

    case 3:
      mode_name = "ch4.mode";
      polarity_name = "ch4.polarity";
      bias_name = "ch4.bias";
      break;
  }

  memcpy(regw.value, &mode, 1);

	int32_t r = wlmio_register_access(node_id, mode_name, &regw, NULL, vpe6060_configure_callback, t);
  if(r < 0)
  {
    free(t);
    goto exit;
  }

  memcpy(regw.value, &polarity, 1);

  r = wlmio_register_access(node_id, polarity_name, &regw, NULL, vpe6060_configure_callback, t);
  if(r < 0)
  {
    t->cancelled += 1;
    goto exit;
  }

  memcpy(regw.value, &bias, 1);

  r = wlmio_register_access(node_id, bias_name, &regw, NULL, vpe6060_configure_callback, t);
  if(r < 0)
  {
    t->cancelled += 1;
    goto exit;
  }

  r = 0;

exit:
	return r;
}


int32_t wlmio_vpe6070_write(const uint8_t node_id, const uint8_t ch, const uint16_t v, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 3 || callback == NULL) { return -EINVAL; }

  struct wlmio_register_access regw;

	regw.type = WLMIO_REGISTER_VALUE_UINT16;
	regw.length = 1;

	memcpy(regw.value, &v, 2);

  const char* name = NULL;
  switch(ch)
  {
    case 0:
      name = "ch1.output";
      break;

    case 1:
      name = "ch2.output";
      break;

    case 2:
      name = "ch3.output";
      break;

    case 3:
      name = "ch4.output";
      break;
  }

	int32_t r = wlmio_register_access(node_id, name, &regw, NULL, callback, uparam);

	return r;
}


struct vpe6080_read
{
  struct wlmio_register_access reg;
  uint16_t* input;
  void (* callback)(int32_t r, void* uparam);
  void* uparam;
};


static void vpe6080_read_callback(int32_t r, void* const p)
{
  struct vpe6080_read* const t = p;

  if(r < 0)
  { goto exit; }

  if(t->reg.type != WLMIO_REGISTER_VALUE_UINT16 || t->reg.length < 1)
  {
    r = -EPROTO;
    goto exit;
  }

  memcpy(t->input, t->reg.value, 2);

  r = 0;

exit:
  t->callback(r, t->uparam);
  free(p);
}


int32_t wlmio_vpe6080_read(const uint8_t node_id, const uint8_t ch, uint16_t* const v, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 7 || v == NULL || callback == NULL) { return -EINVAL; }

  struct vpe6080_read* t = malloc(sizeof(struct vpe6080_read));
  t->input = v;
  t->callback = callback;
  t->uparam = uparam;

  const char* name = NULL;
  switch(ch)
  {
     case 0:
      name = "ch1.input";
      break;

    case 1:
      name = "ch2.input";
      break;

    case 2:
      name = "ch3.input";
      break;

    case 3:
      name = "ch4.input";
      break;

    case 4:
      name = "ch5.input";
      break;

    case 5:
      name = "ch6.input";
      break;

    case 6:
      name = "ch7.input";
      break;

    case 7:
      name = "ch8.input";
      break;
  }
	int32_t r = wlmio_register_access(node_id, name, NULL, &t->reg, &vpe6080_read_callback, t);
  if(r < 0)
  {
    free(t);
    goto exit;
  }

  r = 0;

exit:
	return r;
}


struct vpe6080_configure
{
  uint8_t counter;
  uint8_t cancelled;
  int32_t r;
  void (* callback)(int32_t r, void* uparam);
  void* uparam;
};


static void vpe6080_configure_callback(int32_t r, void* const p)
{
  struct vpe6080_configure* const t = p;

  t->counter += 1;

  if(r < 0 && t->r >= 0)
  { t->r = r; }

  if(t->counter + t->cancelled < 3)
  { return; }

  t->callback(t->r, t->uparam);
  free(p);
}


int32_t wlmio_vpe6080_configure(const uint8_t node_id, const uint8_t ch, uint8_t enabled, const uint16_t beta, const uint16_t t0, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 7 || callback == NULL) { return -EINVAL; }

  enabled = enabled ? 1 : 0;

  struct vpe6080_configure* t = malloc(sizeof(struct vpe6080_configure));
  t->counter = 0;
  t->cancelled = 0;
  t->r = 0;
  t->callback = callback;
  t->uparam = uparam;

  struct wlmio_register_access regw;

	regw.type = WLMIO_REGISTER_VALUE_UINT8;
	regw.length = 1;

  const char* enable_name = NULL;
  const char* beta_name = NULL;
  const char* t0_name = NULL;
  switch(ch)
  {
    case 0:
      enable_name = "ch1.enabled";
      beta_name = "ch1.beta";
      t0_name = "ch1.t0";
      break;

    case 1:
      enable_name = "ch2.enabled";
      beta_name = "ch2.beta";
      t0_name = "ch2.t0";
      break;

    case 2:
      enable_name = "ch3.enabled";
      beta_name = "ch3.beta";
      t0_name = "ch3.t0";
      break;

    case 3:
      enable_name = "ch4.enabled";
      beta_name = "ch4.beta";
      t0_name = "ch4.t0";
      break;

    case 4:
      enable_name = "ch5.enabled";
      beta_name = "ch5.beta";
      t0_name = "ch5.t0";
      break;

    case 5:
      enable_name = "ch6.enabled";
      beta_name = "ch6.beta";
      t0_name = "ch6.t0";
      break;

    case 6:
      enable_name = "ch7.enabled";
      beta_name = "ch7.beta";
      t0_name = "ch7.t0";
      break;

    case 7:
      enable_name = "ch8.enabled";
      beta_name = "ch8.beta";
      t0_name = "ch8.t0";
      break;
  }

  memcpy(regw.value, &enabled, 1);

	int32_t r = wlmio_register_access(node_id, enable_name, &regw, NULL, vpe6080_configure_callback, t);
  if(r < 0)
  {
    free(t);
    goto exit;
  }

  regw.type = WLMIO_REGISTER_VALUE_UINT16;
  memcpy(regw.value, &beta, 2);

  r = wlmio_register_access(node_id, beta_name, &regw, NULL, vpe6080_configure_callback, t);
  if(r < 0)
  {
    t->cancelled += 1;
    goto exit;
  }

  memcpy(regw.value, &t0, 2);

  r = wlmio_register_access(node_id, t0_name, &regw, NULL, vpe6080_configure_callback, t);
  if(r < 0)
  {
    t->cancelled += 1;
    goto exit;
  }

  r = 0;

exit:
	return r;
}


struct node_set_sample_interval
{
  struct wlmio_register_access regr;
  uint16_t sample_interval;
  void (* callback)(int32_t r, void* uparam);
  void* uparam;
};


static void node_set_sample_interval_callback(int32_t r, void* const p)
{
  struct node_set_sample_interval* const t = p;

  if(r < 0)
  { goto exit; }

  if(t->regr.type != WLMIO_REGISTER_VALUE_UINT16 || t->regr.length != 1)
  {
    r = -ENOTSUP;
    goto exit;
  }

  uint16_t sample_interval;
  memcpy(&sample_interval, t->regr.value, 2);

  if(sample_interval != t->sample_interval)
  {
    r = -EBADE;
    goto exit;
  }

  r = 0;

exit:
  t->callback(r, t->uparam);
  free(p);
}


int32_t wlmio_node_set_sample_interval(const uint8_t node_id, const uint16_t sample_interval, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || callback == NULL) { return -EINVAL; }

  struct node_set_sample_interval* t = malloc(sizeof(struct node_set_sample_interval));
  t->callback = callback;
  t->uparam = uparam;

  struct wlmio_register_access regw;

	regw.type = WLMIO_REGISTER_VALUE_UINT16;
	regw.length = 1;
  memcpy(regw.value, &sample_interval, 2);

	int32_t r = wlmio_register_access(node_id, "sample_interval", &regw, &t->regr, node_set_sample_interval_callback, t);
  if(r < 0)
  {
    free(t);
    goto exit;
  }

  r = 0;

exit:
	return r;
}


struct vpe6090_read
{
  struct wlmio_register_access reg;
  uint16_t* input;
  void (* callback)(int32_t r, void* uparam);
  void* uparam;
};


static void vpe6090_read_callback(int32_t r, void* const p)
{
  struct vpe6090_read* const t = p;

  if(r < 0)
  { goto exit; }

  if(t->reg.type != WLMIO_REGISTER_VALUE_UINT16 || t->reg.length < 1)
  {
    r = -EPROTO;
    goto exit;
  }

  memcpy(t->input, t->reg.value, 2);

  r = 0;

exit:
  t->callback(r, t->uparam);
  free(p);
}


int32_t wlmio_vpe6090_read(const uint8_t node_id, const uint8_t ch, uint16_t* const v, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 5 || v == NULL || callback == NULL) { return -EINVAL; }

  struct vpe6090_read* t = malloc(sizeof(struct vpe6090_read));
  t->input = v;
  t->callback = callback;
  t->uparam = uparam;

  const char* name = NULL;
  switch(ch)
  {
     case 0:
      name = "ch1.input";
      break;

    case 1:
      name = "ch2.input";
      break;

    case 2:
      name = "ch3.input";
      break;

    case 3:
      name = "ch4.input";
      break;

    case 4:
      name = "ch5.input";
      break;

    case 5:
      name = "ch6.input";
      break;
  }
	int32_t r = wlmio_register_access(node_id, name, NULL, &t->reg, &vpe6090_read_callback, t);
  if(r < 0)
  {
    free(t);
    goto exit;
  }

  r = 0;

exit:
	return r;
}


int32_t wlmio_vpe6090_configure(const uint8_t node_id, const uint8_t ch, uint8_t type, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if(node_id > CANARD_NODE_ID_MAX || ch > 5 || callback == NULL || type > WLMIO_VPE6090_TYPE_T)
  { return -EINVAL; }

  struct wlmio_register_access regw;

	regw.type = WLMIO_REGISTER_VALUE_UINT8;
	regw.length = 1;

  const char* name = NULL;
  switch(ch)
  {
    case 0:
      name = "ch1.type";
      break;

    case 1:
      name = "ch2.type";
      break;

    case 2:
      name = "ch3.type";
      break;

    case 3:
      name = "ch4.type";
      break;

    case 4:
      name = "ch5.type";
      break;

    case 5:
      name = "ch6.type";
      break;
  }

  memcpy(regw.value, &type, 1);

	int32_t r = wlmio_register_access(node_id, name, &regw, NULL, callback, uparam);

	return r;
}


struct vpe6180_read
{
  struct wlmio_register_access reg;
  uint16_t* input;
  void (* callback)(int32_t r, void* uparam);
  void* uparam;
};


static void vpe6180_read_callback(int32_t r, void* const p)
{
  struct vpe6180_read* const t = p;

  if(r < 0)
  { goto exit; }

  if(t->reg.type != WLMIO_REGISTER_VALUE_UINT16 || t->reg.length < 1)
  {
    r = -EPROTO;
    goto exit;
  }

  memcpy(t->input, t->reg.value, 2);

  r = 0;

exit:
  t->callback(r, t->uparam);
  free(p);
}


int32_t wlmio_vpe6180_read(const uint8_t node_id, const uint8_t ch, uint16_t* const v, void (* const callback)(int32_t r, void* uparam), void* const uparam)
{
  if (node_id > CANARD_NODE_ID_MAX || ch > 7 || v == NULL || callback == NULL)
    return -EINVAL;

  struct vpe6180_read* t = malloc(sizeof(struct vpe6180_read));
  t->input = v;
  t->callback = callback;
  t->uparam = uparam;

  const char* name = NULL;
  switch (ch) {
    case 0:
      name = "ch1.input";
      break;

    case 1:
      name = "ch2.input";
      break;

    case 2:
      name = "ch3.input";
      break;

    case 3:
      name = "ch4.input";
      break;

    case 4:
      name = "ch5.input";
      break;

    case 5:
      name = "ch6.input";
      break;

    case 6:
      name = "ch7.input";
      break;

    case 7:
      name = "ch8.input";
      break;
  }
	int32_t r = wlmio_register_access(node_id, name, NULL, &t->reg, &vpe6180_read_callback, t);
  if(r < 0)
  {
    free(t);
    goto exit;
  }

  r = 0;

exit:
	return r;
}