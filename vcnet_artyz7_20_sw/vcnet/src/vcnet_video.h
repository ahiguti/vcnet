#ifndef VCNET_VIDEO_H
#define VCNET_VIDEO_H

void vcnet_video_init(void);
u8 *vcnet_video_get_frame_data(u32 idx, u32 *width_r, u32 *height_r);
void vcnet_video_restart_video();
u32 vcnet_video_get_video_frame();
void vcnet_video_set_video_frame(u32 vidx, u32 didx);
void vcnet_video_exec_step();
int vcnet_video_status(); /* VIDEO_STREAMING = 1 */
int vcnet_video_locked();

#endif
