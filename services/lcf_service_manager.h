#include <lcf_soh_reset.h>
#include <lcf_cells_reader.h>

struct leaf_service_manager {
	struct lcf_sr soh_rst_fsm;
	struct lcf_cr cells_r_fsm;
};

void leaf_service_manager_init(struct leaf_service_manager *self)
{
	lcf_sr_init(&self->soh_rst_fsm);
	lcf_cr_init(&self->cells_r_fsm);
}
