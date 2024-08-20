#include <SDL_types.h>
#ifdef __cplusplus
extern "C" {
#endif

#define NUM_ROCHES 200
typedef struct
{
    int x;
    int y;
    int obj_3d_id;
}roche;

/// those 2 works as bags function but for "roche" to harvest.
void put_roche_on_ground(int roche_x, int roche_y, int roche_id);
void add_roches_from_list(const Uint8 *data);
void remove_roche(int which_roche);
void remove_all_roches(void);

int  harvest_roche(int object_id);
#ifdef __cplusplus
}
#endif
