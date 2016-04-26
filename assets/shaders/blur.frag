uniform sampler2D u_Texture;

in vec2 v_TexCoords;

out vec4 color;

void main (void)
{
	vec2 tx = v_TexCoords;
#ifdef BLUR_VERTICALLY
	vec2 dx  = vec2 (0, 0.001953);
#elif defined(BLUR_HORIZONTALLY)
	vec2 dx  = vec2 (0.001953, 0);
#endif
	vec2 sdx = dx;
	vec4 sum = texture(u_Texture, tx) * 0.134598;

	sum += (texture(u_Texture, tx + sdx) + texture(u_Texture, tx - sdx)) * 0.127325; 
	sdx += dx;
	sum += (texture(u_Texture, tx + sdx) + texture(u_Texture, tx - sdx)) * 0.107778; 
	sdx += dx;
	sum += (texture(u_Texture, tx + sdx) + texture(u_Texture, tx - sdx)) * 0.081638; 
	sdx += dx;
	sum += (texture(u_Texture, tx + sdx) + texture(u_Texture, tx - sdx)) * 0.055335; 
	sdx += dx;
	sum += (texture(u_Texture, tx + sdx) + texture(u_Texture, tx - sdx)) * 0.033562; 
	sdx += dx;
	sum += (texture(u_Texture, tx + sdx) + texture(u_Texture, tx - sdx)) * 0.018216; 
	sdx += dx;
	sum += (texture(u_Texture, tx + sdx) + texture(u_Texture, tx - sdx)) * 0.008847; 
	sdx += dx;

	color = sum;
}
