#include "ta_texture.h"
#include "ta_log.h"
#include "dlb_types.h"
#include "dlb_memory.h"
#include "dlb_vector.h"

#define STBI_ASSERT(x) DLB_ASSERT(x)
#define STBI_MALLOC dlb_malloc
#define STBI_REALLOC dlb_realloc
#define STBI_FREE dlb_free
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"

static ta_texture_2d *tex[TA_TEXTURE_QUEUE_COUNT];
//static ta_texture_3d *cubemaps[TA_TEXTURE_QUEUE_COUNT];
static GLuint *gl_ids[TA_TEXTURE_QUEUE_COUNT];

ta_texture_2d *ta_texture_init(ta_texture_queue queue, const char *filename)
{
	// Load pixel data from file
	int w, h, channels;
	stbi_set_flip_vertically_on_load(true);
	stbi_uc *pixels = stbi_load(filename, &w, &h, &channels, 4);
	if (!pixels) {
		const char *reason = stbi_failure_reason();
		ta_log_write(tg_debug_log, "Failed to load texture: %s\nSTBI Reason: %s\n",
			filename, reason);
		DLB_ASSERT(!"ta_texture_init: Failed to load texture");
	}

	// Create texture
	ta_texture_2d *texture = dlb_vec_alloc(tex[queue]);
	texture->filename = filename;
	texture->width = w;
	texture->height = h;
	texture->channels = channels;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture->gl_id);
	glBindTexture(GL_TEXTURE_2D, texture->gl_id);
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(pixels);

	GLuint *gl_id = dlb_vec_alloc(gl_ids[queue]);
	*gl_id = texture->gl_id;

	return texture;
}

void ta_texture_clear(ta_texture_queue queue)
{
	glDeleteTextures(dlb_vec_len(gl_ids[queue]), gl_ids[queue]);
	dlb_vec_clear(tex[queue]);
	dlb_vec_clear(gl_ids[queue]);
}