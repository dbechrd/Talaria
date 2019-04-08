#include "misc/gl3w.h"

typedef enum {
	TA_TEXTURE_QUEUE_STATIC,
	TA_TEXTURE_QUEUE_LEVEL,
	TA_TEXTURE_QUEUE_FRAME,
	TA_TEXTURE_QUEUE_COUNT
} ta_texture_queue;

typedef struct {
	const char *filename;
	int width;
	int height;
	int channels;
	GLuint gl_id;
} ta_texture_2d;

ta_texture_2d *ta_texture_init(ta_texture_queue queue, const char *filename);
void ta_texture_clear(ta_texture_queue queue);