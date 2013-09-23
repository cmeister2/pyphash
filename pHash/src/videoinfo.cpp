#include <videoinfo.h>

DefineArray(ulong64);
DefineArray(double);

int videoinfo_init(const char *filename, VIDEOINFO *video_info)
{
  unsigned int ii;
  int err;

  //Check assumptions
  assert(video_info != NULL);

  //Clear the video info
  memset(video_info, 0, sizeof(VIDEOINFO));

  //Store the filename
  video_info->filename = filename;

  //Open the file
  err = avformat_open_input(&(video_info->format_ctx),
                            video_info->filename,
                            NULL,
                            NULL);
  if (err != 0)
  {
    //print_error(filename, err);
    return err;
  }

  err = avformat_find_stream_info(video_info->format_ctx, NULL);
  if (err < 0)
  {
    return err;
  }

  av_dump_format(video_info->format_ctx, 0, video_info->filename, 0);

  // Find the first video stream by looping over the streams in the video
  video_info->stream_id = -1;
  for(ii = 0; ii < video_info->format_ctx->nb_streams; ii++)
  {
    // Look at the codec type; if it's a video stream, stop looking.
    if(video_info->format_ctx->streams[ii]->codec->codec_type ==
                                                            AVMEDIA_TYPE_VIDEO)
    {
      video_info->stream_id = ii;
      video_info->stream = video_info->format_ctx->streams[ii];
      break;
    }
  }

  if(video_info->stream_id == -1)
  {
    return -1; // Didn't find a video stream
  }

  //Setup a pointer to the codec context for the video stream
  video_info->codec_ctx = video_info->stream->codec;
  if (video_info->codec_ctx == NULL)
  {
    return -1;
  }

  // Find the decoder for the video stream
  video_info->codec = avcodec_find_decoder(video_info->codec_ctx->codec_id);
  if(video_info->codec == NULL)
  {
    fprintf(stderr, "Unsupported codec!\n");
    return -1; // Codec not found
  }

  // Open codec
  if(avcodec_open2(video_info->codec_ctx, video_info->codec, NULL) < 0)
  {
    return -1; // Could not open codec
  }

  // Save video width and height information.
  video_info->width = video_info->codec_ctx->width;
  video_info->height = video_info->codec_ctx->height;

  return 0;
}

void videoinfo_term(VIDEOINFO *video_info)
{
  if (video_info->codec_ctx != NULL)
  {
    // Close the codec
    avcodec_close(video_info->codec_ctx);
  }

  if (video_info->format_ctx != NULL)
  {
    // Close the video file
    avformat_close_input(&video_info->format_ctx);
  }
}

int callback_parms_init(CALLBACK_PARMS *parms,
                        VIDEOINFO *vfinfo)
{
  memset(parms, 0, sizeof(CALLBACK_PARMS));

  parms->original = avcodec_alloc_frame();
  if (parms->original == NULL)
  {
    return -1;
  }

  parms->converted = avcodec_alloc_frame();
  if(parms->converted == NULL)
  {
    //Failed to allocate frame
    return -1;
  }

  //Create a context.  This will allow for easy conversion of the frame.
  parms->sws_cxt = sws_getContext(vfinfo->width,
                                  vfinfo->height,
                                  vfinfo->codec_ctx->pix_fmt,
                                  vfinfo->width,
                                  vfinfo->height,
                                  vfinfo->dest_pix_fmt,
                                  SWS_BICUBIC,
                                  NULL,
                                  NULL,
                                  NULL);
  if(parms->sws_cxt == NULL)
  {
    //Failed to allocate frame
    return -1;
  }

  parms->vfinfo = vfinfo;

  return 0;
}

void callback_parms_term(CALLBACK_PARMS *parms)
{
  if (parms->original != NULL)
  {
    av_freep(&parms->original);
  }
  if (parms->converted != NULL)
  {
    av_freep(&parms->converted);
  }
  if (parms->sws_cxt != NULL)
  {
    sws_freeContext(parms->sws_cxt);
  }
}

void each_frame(VIDEOINFO *vfinfo,
                frame_callback *callback,
                void *user_data)
{
  uint8_t *buffer = NULL;
  int buffer_bytes;
  int frame_finished;
  int result;
  AVPacket pkt, pkt1;
  CALLBACK_PARMS parms;
  int ret;

  //Check assumptions.
  assert(callback != NULL);

  //Initialize the callback parameters as much as possible.
  if(callback_parms_init(&parms, vfinfo) != 0)
  {
    goto EXIT_LABEL;
  }

  parms.user_data = user_data;

  //Calculate how many bytes are needed to store one frame of this video.
  buffer_bytes = avpicture_get_size(vfinfo->dest_pix_fmt,
                                    vfinfo->width,
                                    vfinfo->height);

  //Allocate a buffer for storing a frame
  buffer=(uint8_t *)av_malloc(buffer_bytes*sizeof(uint8_t));
  if (buffer == NULL)
  {
    goto EXIT_LABEL;
  }

  //Fill in the data in the destination frame
  avpicture_fill((AVPicture *)parms.converted,
                 buffer,
                 vfinfo->dest_pix_fmt,
                 vfinfo->width,
                 vfinfo->height);

  //Initialize a packet for reading frames
  av_init_packet(&pkt);

  result = 1;
  while (result >= 0)
  {
    //Read a frame into the packet structure.
    result =  av_read_frame(vfinfo->format_ctx, &pkt);
    if (result < 0)
    {
      //No more frames left.
      break;
    }

    //Check if the packet matches our video stream.
    if (pkt.stream_index == vfinfo->stream_id)
    {
      //Copy the packet for decoding.
      pkt1 = pkt;
      ret = 1;

      while (pkt1.size && ret)
      {
        ret = 0;
        avcodec_get_frame_defaults(parms.original);

        //Decode the video into original_frame.
        ret = avcodec_decode_video2(vfinfo->codec_ctx,
                                    parms.original,
                                    &frame_finished,
                                    &pkt1);
        if (ret < 0)
        {
          continue;
        }

        ret = FFMIN(ret, pkt1.size); /* guard against bogus return values */
        pkt1.data += ret;
        pkt1.size -= ret;
        if (frame_finished)
        {
          callback(&parms);
        }

        ret = frame_finished;
      }
    }

    av_free_packet(&pkt);
  }

EXIT_LABEL:

  callback_parms_term(&parms);
  if (buffer != NULL)
  {
    av_freep(&buffer);
  }
}

int vi_hash_from_cimg(CImg<uint8_t>* src,ulong64 &hash){

    CImg<float> meanfilter(7,7,1,1,1);
    CImg<float> img;
    if (src->spectrum() == 3){
        img = src->RGBtoYCbCr().channel(0).get_convolve(meanfilter);
    } else if (src->spectrum() == 4){
        int width = img.width();
        int height = img.height();
        int depth = img.depth();
        img = src->crop(0,0,0,0,width-1,height-1,depth-1,2).RGBtoYCbCr().channel(0).get_convolve(meanfilter);
    } else {
        img = src->channel(0).get_convolve(meanfilter);
    }

    img.resize(32,32);
    CImg<float> *C  = ph_dct_matrix(32);
    CImg<float> Ctransp = C->get_transpose();

    CImg<float> dctImage = (*C)*img*Ctransp;

    CImg<float> subsec = dctImage.crop(1,1,8,8).unroll('x');;

    float median = subsec.median();
    ulong64 one = 0x0000000000000001;
    hash = 0x0000000000000000;
    for (int i=0;i< 64;i++){
        float current = subsec(i);
        if (current > median)
            hash |= one;
        one = one << 1;
    }

    delete C;

    return 0;
}

void vi_keyframes(CALLBACK_PARMS *parms)
{
  CImg<float> Ctransp;
  CImg<uint8_t> next_image;
  int channels;
  HASHDATA *tdata = (HASHDATA *)parms->user_data;

  if (!parms->original->key_frame)
  {
    return;
  }

  sws_scale(parms->sws_cxt,
            parms->original->data,
            parms->original->linesize,
            0,
            parms->vfinfo->height,
            parms->converted->data,
            parms->converted->linesize);

  //Calculate the number of channels that the image will have.
  channels = ((parms->vfinfo->dest_pix_fmt == PIX_FMT_GRAY8) ? 1 : 3);

  next_image.assign(*parms->converted->data,
                    channels,
                    parms->vfinfo->width,
                    parms->vfinfo->height,
                    1,
                    false);

  next_image.permute_axes("yzcx");

  // Store the image in the framelist
  //tdata->framelist->push_back(next_image);

  //Get the image hash
  ulong64 hash = 0x0000000000000000;
  vi_hash_from_cimg(&next_image, hash);

  // Store the frame info in the array
  double pts = parms->original->pkt_pts *
                                      av_q2d(parms->vfinfo->stream->time_base);
  insert_doubleArray(&tdata->pts, pts);
  insert_ulong64Array(&tdata->hashes, hash);

  // Tick up the count of keyframes
  tdata->keyframes++;

  /*printf("Keyframe %d: hash %llu pts %f secs\n",
         tdata->keyframes,
         hash,
         pts);*/
}

HASHDATA *ph_max_videohash(char *filename)
{
  VIDEOINFO vfinfo;
  int rc;
  HASHDATA *tdata;

  tdata = (HASHDATA *)malloc(sizeof(HASHDATA));

  //Register AV format
  av_register_all();

  //Get video information.
  rc = videoinfo_init(filename, &vfinfo);
  if (rc != 0)
  {
     goto EXIT_LABEL;
  }

  vfinfo.dest_pix_fmt = PIX_FMT_RGB24;

  //Initialize testdata
  //tdata.framelist = new CImgList<uint8_t>();
  tdata->keyframes = 0;
  init_doubleArray(&tdata->pts, 400);
  init_ulong64Array(&tdata->hashes, 400);

  //Video information was good.  Iterate through the video, saving the
  //keyframes and generating the hashes.
  each_frame(&vfinfo, vi_keyframes, tdata);

//  free_doubleArray(&tdata->pts);
//  free_ulong64Array(&tdata->hashes);

  videoinfo_term(&vfinfo);

EXIT_LABEL:
  return tdata;
}
