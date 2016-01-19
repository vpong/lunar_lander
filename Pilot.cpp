#include <float.h>
#include "Pilot.hpp"
#include "World.hpp"
#include "Lander.hpp"
#include "Ground.hpp"
#include "constants.hpp"
#include "Utils.hpp"

Pilot::Pilot() {
    state = BEGIN;
    rot_state = START;
    frame = 0;
}

// rotate the lander to the target orientation
// to use:
// rotate_to(l, angle)
// if (rot_state == DONE) Go to next state; rot_state = START
void Pilot::rotate_to(Lander& l, float tgt_orientation) {
    if (rot_state == START) {
        // choose which direction to rotate
        init_orientation = l.orientation;
        bool through_zero = Utils::angle_diff(tgt_orientation,
                                              init_orientation,
                                              &d_theta
        );
        int correction = through_zero ? -1 : 1;

        if (tgt_orientation > l.orientation) {
            l.torque = l.max_torque * correction;
        } else {
            l.torque = -l.max_torque * correction;
        }
        rot_state = TORQUE_UP;
        rot_frame = 1;
    } else if (rot_state == TORQUE_UP) {
        // torque until we make it halfway
        float traveled;
        Utils::angle_diff(init_orientation, l.orientation, &traveled);
        if (traveled >= d_theta / 2.) {
            l.torque *= -1;
            rot_state = TORQUE_DOWN;
            flip_frame = rot_frame;
            rot_frame = 1;
        } else {
            rot_frame++;
        }
    } else if (rot_state == TORQUE_DOWN) {
        // null out spin by applying down torque exactly as many frames as we
        // applied up torque
        if (rot_frame >= flip_frame) {
            l.torque *= 0;
            rot_state = DONE;

            float diff;
            Utils::angle_diff(l.orientation, tgt_orientation, &diff);
            printf("rot error = %f radians\n", diff);
        } else {
            rot_frame++;
        }
    }
}

// dist_to_pad in m/s (not pixels)
float Pilot::fall_time(float y_vel, float dist_to_pad) {
    // d = v_0 * t + (g/2) * t^2
    // Solve with quadratic method
    float a = World::g / 2;
    float b = y_vel;
    float c = -dist_to_pad;
    return (-b + sqrt(b * b - 4. * a * c)) / (2. * a);
}

void Pilot::fly(Lander& l, World& world) {
    l.thrust = l.max_thrust;

    Ground& pad = world.grounds.at(0);
    float dist_to_pad = (pad.begin.y - (l.y_pos + l.COLLISION_HEIGHT)) /
                        l.pixels_per_meter;
    if (state == BEGIN) {
        float fall_t = fall_time(l.y_vel, dist_to_pad);
        float x_pos_pred = l.x_pos + l.rot_abt.x + l.x_vel * fall_t * l.pixels_per_meter;
        if (x_pos_pred < pad.begin.x) {
            state = ROTATION1;
            target_orientation = 0.;
        } else if (x_pos_pred > pad.get_right()) {
            state = ROTATION1;
            target_orientation = M_PI;
        } else {
            state = BEFORE_Y_BURN;
        }
    } else if (state == ROTATION1) {
        rotate_to(l, target_orientation);
        if (rot_state == DONE) {
            rot_state = START;
            state = INITIATE_X_BURN1;
            target_orientation -= M_PI;
            if (target_orientation < 0.) {
                target_orientation += 2 * M_PI;
            }
        }
    } else if (state == INITIATE_X_BURN1) {
        l.thrusting = true;
        float x_dist_from_pad =
            (float) abs(l.x_pos + l.rot_abt.x - pad.get_center()) /
            l.pixels_per_meter;
        float target_x_vel = x_dist_from_pad / PI_FLIP_TIME;
        float delta_x_vel = fabs(l.x_vel) + target_x_vel;
        // dt = dv * m / F_T
        float burn_time = delta_x_vel * (l.dry_mass + l.fuel) /
                          (l.thrust * fabs(cos(l.orientation)));
        printf("burn for %f seconds = ", burn_time);
        int burn_frames =
            Utils::round_nearest_int(burn_time * 1000. / FRAME_TIME);
        printf("%d frames\n", burn_frames);
        stop_burn_frame = frame + burn_frames;
        state = X_BURN1;
    } else if (state == X_BURN1) {
        if (frame >= stop_burn_frame) {
            l.thrusting = false;
            state = ROTATION2;
        }
    } else if (state == ROTATION2) {
        rotate_to(l, target_orientation);
        if (rot_state == DONE) {
            rot_state = START;
            state = INITIATE_X_BURN2;
        }
    } else if (state == INITIATE_X_BURN2) {
        l.thrusting = true;
        float burn_time = fabs(l.x_vel) * (l.dry_mass + l.fuel) /
                          (l.thrust * fabs(cos(l.orientation)));
        int burn_frames =
            Utils::round_nearest_int(burn_time * 1000. / FRAME_TIME);
        stop_burn_frame = frame + burn_frames;
        state = X_BURN2;
    } else if (state == X_BURN2) {
        if (frame >= stop_burn_frame) {
            l.thrusting = false;
            state = ROTATION3;
        }
    } else if (state == ROTATION3) {
        rotate_to(l, 3 * M_PI_2);
        if (rot_state == DONE) {
            rot_state = START;
            state = BEFORE_Y_BURN;
        }
    } else if (state == BEFORE_Y_BURN) {
        // When do we turn on the thruster?
        // What will our acceleration be while thrusting?
        // a = g - F_T / m                 (NOTE: assumption dm/dt << m)
        float net_accel = World::g + l.thrust * sin(l.orientation) /
                                     (l.dry_mass + l.fuel);
        // How long will that thrust take?
        // t = v / a
        float thrust_time = fabs(l.y_vel / net_accel);
        // How far from pad must we begin burning to burn for t time?
        // d = v * t + (a/2) * t^2
        float thrust_distance = l.y_vel * thrust_time +
            .5 * net_accel * thrust_time * thrust_time;
        if (dist_to_pad <= thrust_distance) {
            int burn_frames =
                Utils::round_nearest_int(thrust_time * 1000. /
                                         (float) FRAME_TIME);
            l.thrusting = true;
            state = Y_BURN;
            stop_burn_frame = frame + burn_frames;
        }
    } else if (state == Y_BURN) {
        if (frame >= stop_burn_frame) {
            l.thrusting = false;
            state = AFTER_Y_BURN;
        }
    }
    frame++;
}
