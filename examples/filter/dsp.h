#ifndef DSP_H
#define DSP_H

struct filter
{
	float cutoff;
	float resonance;
	float pos;
	float vel;
};

static inline float filter_lp_iir( float in, struct filter *filter )
{
	filter->vel += ( in - filter->pos ) * filter->cutoff;
	filter->pos += filter->vel;
	filter->vel *= filter->resonance;
	return filter->pos;
}

static inline float filter_hp_iir( float in, struct filter *filter )
{
	filter->vel += ( in - filter->pos ) * filter->cutoff;
	filter->pos += filter->vel;
	filter->vel *= filter->resonance;
	return in-filter->pos;
}

static inline float filter_lp_fir( float in, struct filter *filter )
{
	const float cutoff = filter->cutoff*0.5f+0.5;
	float out = in*cutoff+filter->pos*(1.0f-cutoff);
	filter->pos = in;
	return out;
}

#endif
