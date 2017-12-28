#include "wave_trans_send.h"
#include "transceiver/send/wt_send_link_layer.h"
#include "transceiver/send/wt_send_physical_layer.h"
#include "proto_utils/wt_proto_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  WTSendLinkHander      *link_hander_;
  WTSendPhyHander       *phy_hander_;
  WaveTransPcmInfo      pcm_info_;
}WaveTransSendHanderData;

static int WaveTransSendAttrCheck(WaveTransSendAttr *attr)
{
  if (attr == NULL) {
    return -1;
  }
  if (attr->sample_bit_ != 16 && attr->sample_bit_ != 8) {
    return -1;
  }
  if (attr->sample_rate_ / 2 < MAX_FREQ) {
    return -1;
  }
  return 0;
}

static WaveTransSendHanderData * WaveTransSendHanderDataInit(WaveTransSendAttr *attr)
{
  WaveTransSendHanderData *hander_data = (WaveTransSendHanderData *)malloc(sizeof(WaveTransSendHanderData));
  if (hander_data == NULL) {
    return NULL;
  }
  hander_data->link_hander_ = WTSendLinkLayerCreateHander();
  if (hander_data->link_hander_ == NULL) {
    free(hander_data);
    return NULL;
  }
  WTSendPhyHanderAttr phy_attr;
  phy_attr.sample_bit_ = attr->sample_bit_;
  phy_attr.sample_rate_ = attr->sample_rate_;
  hander_data->phy_hander_ = WTSendPhyLayerCreatHander(&phy_attr);
  if (hander_data->phy_hander_ == NULL) {
    WTSendLinkLayerDestroyHander(hander_data->link_hander_);
    free(hander_data);
    return NULL;
  }
  hander_data->pcm_info_.pcm_buff_ = NULL;
  hander_data->pcm_info_.buff_len_ = 0;
  return hander_data;
}

static void WaveTransSendHanderDataExit(WaveTransSendHanderData *hander_data)
{
  WTSendLinkLayerDestroyHander(hander_data->link_hander_);
  hander_data->link_hander_ = NULL;
  WTSendPhyLayerDestroyHander(hander_data->phy_hander_);
  hander_data->link_hander_ = NULL;
  if (hander_data->pcm_info_.pcm_buff_ != NULL) {
    free(hander_data->pcm_info_.pcm_buff_);
    hander_data->pcm_info_.pcm_buff_ = NULL;
  }
  free(hander_data);
}



WaveTransSendHander * WaveTransSendCreateHander(WaveTransSendAttr * attr)
{
  if (WaveTransSendAttrCheck(attr) != 0) {
    return NULL;
  }
  WaveTransSendHanderData *hander_data = WaveTransSendHanderDataInit(attr);
  if (hander_data == NULL) {
    return NULL;
  }
  WaveTransSendHander *hander = (WaveTransSendHander *)malloc(sizeof(WaveTransSendHander));
  if (hander == NULL) {
    WaveTransSendHanderDataExit(hander_data);
    return NULL;
  }
  hander->data_ = hander_data;
  return hander;
}

void WaveTransSendDestroyHander(WaveTransSendHander * hander)
{
  WaveTransSendHanderData *hander_data = (WaveTransSendHanderData *)hander->data_;
  WaveTransSendHanderDataExit(hander_data);
  free(hander);
}

WaveTransPcmInfo * WaveTransSendGetPcm(WaveTransSendHander *hander, void * context, int context_len)
{
  WaveTransSendHanderData *hander_data = (WaveTransSendHanderData *)hander->data_;
  WTSendLinkPackageS *packages = WTSendLinkLayerGetPackage(hander_data->link_hander_, context, context_len);
  if (packages == NULL) {
    return NULL;
  }
  WTSendPcmBuffType *pcm_type = WTSendPhyLayerGetPcm(hander_data->phy_hander_, packages);
  if (pcm_type == NULL) {
    WTSendLinkLayerReleasePackage(hander_data->link_hander_);
    return NULL;
  }
  if (hander_data->pcm_info_.pcm_buff_ != NULL) {
    free(hander_data->pcm_info_.pcm_buff_);
    hander_data->pcm_info_.pcm_buff_ = NULL;
  }
  hander_data->pcm_info_.pcm_buff_ = malloc(pcm_type->buff_len_);
  if (hander_data->pcm_info_.pcm_buff_ == NULL) {
    WTSendPhyLayerReleasePcm(hander_data->phy_hander_);
    WTSendLinkLayerReleasePackage(hander_data->link_hander_);
    return NULL;
  }
  memcpy(hander_data->pcm_info_.pcm_buff_, pcm_type->buff_, pcm_type->buff_len_);
  hander_data->pcm_info_.buff_len_ = pcm_type->buff_len_;
  hander_data->pcm_info_.sample_bit_ = pcm_type->sample_bit_;
  hander_data->pcm_info_.sample_rate_ = pcm_type->sample_rate_;
  WTSendPhyLayerReleasePcm(hander_data->phy_hander_);
  WTSendLinkLayerReleasePackage(hander_data->link_hander_);
  return &hander_data->pcm_info_;
}


#ifdef __cplusplus
}
#endif