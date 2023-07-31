#include <ext.h>
#include <ext_obex.h>
#include <ext_mess.h>
#include <ext_strings.h>
#include <math.h>

//channels of the multislider
#define DIMMER_CHANNEL_IN 1
#define STROBE_CHANNEL_IN 2
#define R_CHANNEL_IN 3
#define G_CHANNEL_IN 4
#define B_CHANNEL_IN 5

//channels of the specific DMX light
#define DIMMER_CHANNEL_OUT 4
#define STROBE_CHANNEL_OUT 5
#define R_CHANNEL_OUT 1
#define G_CHANNEL_OUT 2
#define B_CHANNEL_OUT 3

#define STROBE_STOP 0
#define STROBE_SLOW_CODE 20
#define STROBE_FAST_CODE 75
#define DIMMER_ONOFF_CODE 254
#define DIMMER_OFF 0


typedef struct _alumia_dmx_controller { //estrutura que define o objecto

	t_object s_obj;

	//outlet
	void* m_outlet;

	//inlet capable of getting multiple information
	long m_in;
	void* m_proxy;

	//channel memory
	long dimmer_value;
	int dimmer_on_off;
	long strobe_value;
	int strobe_on_off;
	long previous_channel;

	long strobe_last_value;
	long dimmer_last_value;
	int dim_or_strobe;

	double delay;

	//clock that refers to the scheduller
	void* m_clock;

	//output
	t_atom list_out[2];

} t_alumia_dmx_controller;

static t_class* s_alumia_dmx_controller_class;

void* alumia_dmx_controller_new();
void alumia_dmx_controller_list(t_alumia_dmx_controller*, t_symbol*, long, t_atom*);
void alumia_dmx_controller_joy_in(t_alumia_dmx_controller*, t_symbol*, long, t_atom*);
void dn_delayed_action(t_alumia_dmx_controller*);
void dn_init_values(t_alumia_dmx_controller*);

void ext_main(void* r)
{

	t_class* c;
	c = class_new("alumia_dmx_controller", (method)alumia_dmx_controller_new, (method)NULL, sizeof(t_alumia_dmx_controller), 0L, 0);

	class_addmethod(c, (method)alumia_dmx_controller_list, "list", A_GIMME, 0);

	class_addmethod(c, (method)alumia_dmx_controller_joy_in, "anything", A_GIMME, 0);

	class_register(CLASS_BOX, c);

	s_alumia_dmx_controller_class = c;

}

void* alumia_dmx_controller_new()
{
	t_alumia_dmx_controller* x = (t_alumia_dmx_controller*)object_alloc(s_alumia_dmx_controller_class);

	x->m_clock = clock_new((t_object*)x, (method)dn_delayed_action);
	
	x->m_proxy = proxy_new((t_object*)x, 1, &x->m_in);

	x->m_outlet = listout(x);

	dn_init_values(x);

	return x;

}

void alumia_dmx_controller_list(t_alumia_dmx_controller* x, t_symbol* s, long argc, t_atom* argv)
{

	t_atom* list_values = argv;
	long channel = atom_getlong(list_values);
	long intensity = atom_getlong(list_values + 1);

	//verificar se o input é strobe ou não
	switch (channel)
	{
	case STROBE_CHANNEL_IN:
		if (x->strobe_on_off == 0)//estava desligado, é para ligar
		{
			if (intensity == STROBE_FAST_CODE)//passa a strobe rápido
			{
				atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
				atom_setlong(x->list_out + 1, STROBE_FAST_CODE);
				x->strobe_last_value = STROBE_FAST_CODE;

				if (x->dimmer_on_off == 0)//luz está desligada
				{
					outlet_list(x->m_outlet, NULL, 2, x->list_out);

					atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);
					atom_setlong(x->list_out + 1, x->dimmer_value);

					clock_fdelay(x->m_clock, x->delay);

					x->strobe_on_off = 1;
					x->strobe_value = STROBE_FAST_CODE;

				}
				else//luz já está ligada
				{
					outlet_list(x->m_outlet, NULL, 2, x->list_out);

					x->strobe_on_off = 1;
					x->strobe_value = STROBE_FAST_CODE;
				}
			}
			else if (intensity == STROBE_SLOW_CODE)
			{
				atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
				atom_setlong(x->list_out + 1, STROBE_SLOW_CODE);
				x->strobe_last_value = STROBE_SLOW_CODE;

				if (x->dimmer_on_off == 0)//luz está desligada
				{
					outlet_list(x->m_outlet, NULL, 2, x->list_out);

					atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);
					atom_setlong(x->list_out + 1, x->dimmer_value);

					clock_fdelay(x->m_clock, x->delay);

					x->strobe_on_off = 1;
					x->strobe_value = STROBE_SLOW_CODE;
				}
				else//luz já está ligada
				{
					outlet_list(x->m_outlet, NULL, 2, x->list_out);

					x->strobe_on_off = 1;
					x->strobe_value = STROBE_SLOW_CODE;
				}
			}
			else//caso em que não é nenhum dos códigos (está a ser alterado com o joystick)
			{
				atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
				atom_setlong(x->list_out + 1, intensity);

				outlet_list(x->m_outlet, NULL, 2, x->list_out);

				x->strobe_last_value = intensity;
				x->strobe_on_off = 1;
			}
		}
		else//já estava ligado
		{
			if (intensity == STROBE_FAST_CODE)//voltou-se a carregou-se strobe rápido
			{
				if (x->strobe_value == STROBE_FAST_CODE)//tinha-se estado no strobe rápido e variou-se com o jostick, é para desligar
				{
					if (x->dimmer_on_off == 0)//luz estava desligada
					{
						atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, DIMMER_OFF);

						outlet_list(x->m_outlet, NULL, 2, x->list_out);

						atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, STROBE_STOP);

						clock_fdelay(x->m_clock, x->delay);
						x->strobe_last_value = STROBE_STOP;
						x->strobe_on_off = 0;
					}
					else//luz já está ligada
					{
						atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, STROBE_STOP);

						outlet_list(x->m_outlet, NULL, 2, x->list_out);
						x->strobe_last_value = STROBE_STOP;
						x->strobe_on_off = 0;
					}

					x->strobe_value = -1;
				}
				else//não se tinha estado no strobe rápida, é para ir para lá
				{
					if (x->dimmer_on_off == 0)//luz estava desligada
					{
						atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, STROBE_FAST_CODE);

						outlet_list(x->m_outlet, NULL, 2, x->list_out);

						atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, x->dimmer_value);

						clock_fdelay(x->m_clock, x->delay);

						x->strobe_value = STROBE_FAST_CODE;
						x->strobe_last_value = STROBE_FAST_CODE;
						x->strobe_on_off = 1;
					}
					else//luz já está ligada
					{
						atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, STROBE_FAST_CODE);

						outlet_list(x->m_outlet, NULL, 2, x->list_out);

						x->strobe_value = STROBE_FAST_CODE;
						x->strobe_last_value = STROBE_FAST_CODE;
						x->strobe_on_off = 1;
					}
				}
			}
			else if (intensity == STROBE_SLOW_CODE)//voltou-se a carregar no strobe lento
			{
				if (x->strobe_value == STROBE_SLOW_CODE)//tinha-se estado no strobe lento e variou-se com o jostick, é para desligar
				{
					if (x->dimmer_on_off == 0)//luz estava desligada
					{
						atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, DIMMER_OFF);

						outlet_list(x->m_outlet, NULL, 2, x->list_out);

						atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, STROBE_STOP);

						clock_fdelay(x->m_clock, x->delay);
						x->strobe_last_value = STROBE_STOP;
						x->strobe_on_off = 0;
					}
					else//luz já está ligada
					{
						atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, STROBE_STOP);

						outlet_list(x->m_outlet, NULL, 2, x->list_out);
						x->strobe_last_value = STROBE_STOP;
						x->strobe_on_off = 0;
					}

					x->strobe_value = -1;
				}
				else//não se tinha estado no strobe lento, é para ir para lá
				{
					if (x->dimmer_on_off == 0)//luz estava desligada
					{
						atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, STROBE_SLOW_CODE);

						outlet_list(x->m_outlet, NULL, 2, x->list_out);

						atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, x->dimmer_value);

						clock_fdelay(x->m_clock, x->delay);
						x->strobe_last_value = STROBE_SLOW_CODE;
						x->strobe_value = STROBE_SLOW_CODE;
						x->strobe_on_off = 1;
					}
					else//luz já está ligada
					{
						atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
						atom_setlong(x->list_out + 1, STROBE_SLOW_CODE);

						outlet_list(x->m_outlet, NULL, 2, x->list_out);
						x->strobe_last_value = STROBE_SLOW_CODE;
						x->strobe_value = STROBE_SLOW_CODE;
						x->strobe_on_off = 1;
					}
				}
			}
			else//quer-se mudar a velocidade
			{
				atom_setlong(x->list_out, STROBE_CHANNEL_OUT);
				atom_setlong(x->list_out + 1, intensity);
				x->strobe_last_value = intensity;
				outlet_list(x->m_outlet, NULL, 2, x->list_out);
			}

		}
		x->previous_channel = channel;
		break;

	case DIMMER_CHANNEL_IN:
		if (intensity == DIMMER_ONOFF_CODE && x->dimmer_on_off == 1)//dimmer está ligado, é para desligar
		{

			atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);
			atom_setlong(x->list_out + 1, DIMMER_OFF);

			x->dimmer_on_off = 0;
			x->dimmer_last_value = DIMMER_OFF;
			outlet_list(x->m_outlet, NULL, 2, x->list_out);

		}
		else if (intensity == DIMMER_ONOFF_CODE && x->dimmer_on_off == 0)//dimmer está desligado, é para ligar
		{

			atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);
			atom_setlong(x->list_out + 1, x->dimmer_value);
			x->dimmer_last_value = x->dimmer_value;
			x->dimmer_on_off = 1;

			outlet_list(x->m_outlet, NULL, 2, x->list_out);

		}
		else if (intensity != DIMMER_ONOFF_CODE && x->dimmer_on_off == 0)//está desligado mas queremos alterar o valor quando ficar on
		{
			
			x->dimmer_value = intensity;
			x->dimmer_last_value = x->dimmer_value;

		}
		else//está ligado e queremos alterar o valor
		{

			atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);
			atom_setlong(x->list_out + 1, intensity);

			x->dimmer_value = intensity;
			x->dimmer_last_value = intensity;
			outlet_list(x->m_outlet, NULL, 2, x->list_out);

		}
		x->previous_channel = channel;
		break;

	case R_CHANNEL_IN:

		atom_setlong(x->list_out, R_CHANNEL_OUT);
		atom_setlong(x->list_out + 1, intensity);

		outlet_list(x->m_outlet, NULL, 2, x->list_out);
		break;

	case G_CHANNEL_IN:

		atom_setlong(x->list_out, G_CHANNEL_OUT);
		atom_setlong(x->list_out + 1, intensity);

		outlet_list(x->m_outlet, NULL, 2, x->list_out);
		break;

	case B_CHANNEL_IN:

		atom_setlong(x->list_out, B_CHANNEL_OUT);
		atom_setlong(x->list_out + 1, intensity);

		outlet_list(x->m_outlet, NULL, 2, x->list_out);
		break;

	default:
		post("Canal nao reconhecido");
	}
}

void alumia_dmx_controller_joy_in(t_alumia_dmx_controller* x, t_symbol* s, long argc, t_atom* argv)
{
	t_atom* list_values = argv;
	t_symbol* code_j = gensym("jl");
	t_symbol* code_c = gensym("cj");

	long value = atom_getlong(list_values);

	if (s->s_name == code_j->s_name)
	{

		if (x->dim_or_strobe == DIMMER_CHANNEL_IN)
		{
				
			atom_setlong(x->list_out, DIMMER_CHANNEL_OUT);

			if (value == 1 && x->dimmer_last_value < 254)
			{
				atom_setlong(x->list_out + 1, x->dimmer_last_value + 1);
				x->dimmer_last_value += 1;
				x->dimmer_value = x->dimmer_last_value;

				outlet_list(x->m_outlet, NULL, 2, x->list_out);
			}
			else if (value == -1 && x->dimmer_last_value > 0)
			{
				atom_setlong(x->list_out + 1, x->dimmer_last_value - 1);
				x->dimmer_last_value -= 1;
				x->dimmer_value = x->dimmer_last_value;
				outlet_list(x->m_outlet, NULL, 2, x->list_out);
			}
		
		}
		else
		{
			atom_setlong(x->list_out, STROBE_CHANNEL_OUT);

			if (value == 1 && x->strobe_last_value < 99)
			{
				atom_setlong(x->list_out + 1, x->strobe_last_value + 1);
				x->strobe_last_value += 1;

				outlet_list(x->m_outlet, NULL, 2, x->list_out);
			}
			else if (value == -1 && x->strobe_last_value > 8)
			{
				atom_setlong(x->list_out + 1, x->strobe_last_value - 1);
				x->strobe_last_value -= 1;
				outlet_list(x->m_outlet, NULL, 2, x->list_out);
			}

		}

	}
	else if (s->s_name == code_c->s_name)
	{
		if(x->dim_or_strobe == DIMMER_CHANNEL_IN)
		{
			x->dim_or_strobe = STROBE_CHANNEL_IN;
		}
		else
		{
			x->dim_or_strobe = DIMMER_CHANNEL_IN;
		}
	}

}
void dn_delayed_action(t_alumia_dmx_controller* x)
{

	outlet_list(x->m_outlet, NULL, 2, x->list_out);

}

void dn_init_values(t_alumia_dmx_controller* x)
{
	
	x->dimmer_on_off = 0;
	x->strobe_on_off = 0;
	x->dimmer_value = 254;
	x->strobe_value = -1;
	x->previous_channel = DIMMER_CHANNEL_IN;

	x->dimmer_last_value = 254;
	x->strobe_last_value = -1;

	x->dim_or_strobe = DIMMER_CHANNEL_IN;

	x->delay = 50;
}