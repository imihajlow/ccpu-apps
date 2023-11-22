use font_kit::family_name::FamilyName;
use font_kit::properties::{Properties, Weight};
use font_kit::source::SystemSource;
use minifb::{Key, KeyRepeat, Window, WindowOptions};
use net::{NetListener, NetMessage};
use pong::{Pong, StepResult};
use raqote::{DrawOptions, DrawTarget, Point, SolidSource, Source};
use std::{
    io,
    time::{Duration, SystemTime},
};

mod net;
mod pong;

#[macro_use]
extern crate num_derive;

pub const PIXEL_PER_SQUARE: usize = 10;

const WIDTH: usize = 80 * PIXEL_PER_SQUARE;
const HEIGHT: usize = 60 * PIXEL_PER_SQUARE;

enum AppState {
    Lobby,
    Score,
    Game(BallBoard),
}

fn lobby_process(window: &mut Window, listener: &NetListener) -> io::Result<bool> {
    let size = window.get_size();
    let mut dt = DrawTarget::new(size.0 as i32, size.1 as i32);
    while window.is_open() {
        match listener.get_message()? {
            Some(NetMessage::Announce) => return Ok(false),
            Some(NetMessage::Ready) => return Ok(false),
            _ => (),
        };
        window
            .update_with_buffer(dt.get_data(), size.0, size.1)
            .unwrap();
    }
    Ok(true)
}

struct BallBoard {
    ball_x: i16,
    ball_y: i16,
    ball_speed_x: i16,
    ball_speed_y: i16,
    board_row: u8,
}

fn score_process(window: &mut Window, listener: &NetListener) -> io::Result<Option<BallBoard>> {
    let mut left_ready = false;
    let mut right_ready = false;

    let font = SystemSource::new()
        .select_best_match(
            &[FamilyName::Serif],
            &Properties::new().weight(Weight::MEDIUM),
        )
        .unwrap()
        .load()
        .unwrap();

    let size = window.get_size();
    let mut dt = DrawTarget::new(size.0 as i32, size.1 as i32);
    let mut last_msg_time = SystemTime::now();
    let mut score_left = 0;
    let mut score_right = 0;
    while window.is_open() {
        if window.is_key_pressed(Key::Enter, KeyRepeat::No) {
            right_ready = true;
        }
        let now = SystemTime::now();
        let duration = now.duration_since(last_msg_time).unwrap();
        if duration > Duration::from_secs(1) {
            if right_ready {
                listener.send_ready()?;
            } else {
                listener.send_join()?;
            }
            last_msg_time = now;
        }
        match listener.get_message()? {
            Some(NetMessage::Ready) => {
                left_ready = true;
            }
            Some(NetMessage::BallBoard {
                ball_x,
                ball_y,
                ball_speed_x,
                ball_speed_y,
                board_row,
            }) => {
                if right_ready {
                    return Ok(Some(BallBoard {
                        ball_x,
                        ball_y,
                        ball_speed_x,
                        ball_speed_y,
                        board_row,
                    }));
                }
            }
            Some(NetMessage::Score { left, right }) => {
                score_left = left;
                score_right = right;
            }
            _ => (),
        };

        dt.clear(SolidSource::from_unpremultiplied_argb(0xff, 0x0, 0x0, 0x0));
        dt.draw_text(
            &font,
            24.,
            &format!("{}", score_left),
            Point::new(30., 30.),
            &Source::Solid(SolidSource {
                r: 0xff,
                g: 0xff,
                b: 0xff,
                a: 0xff,
            }),
            &DrawOptions::new(),
        );
        dt.draw_text(
            &font,
            24.,
            &format!("{}", score_right),
            Point::new(400., 30.),
            &Source::Solid(SolidSource {
                r: 0xff,
                g: 0xff,
                b: 0xff,
                a: 0xff,
            }),
            &DrawOptions::new(),
        );
        dt.draw_text(
            &font,
            24.,
            if left_ready { "Ready!" } else { "Ready?" },
            Point::new(30., 300.),
            &Source::Solid(SolidSource {
                r: 0xff,
                g: 0xff,
                b: 0xff,
                a: 0xff,
            }),
            &DrawOptions::new(),
        );
        dt.draw_text(
            &font,
            24.,
            if right_ready { "Ready!" } else { "Ready?" },
            Point::new(400., 300.),
            &Source::Solid(SolidSource {
                r: 0xff,
                g: 0xff,
                b: 0xff,
                a: 0xff,
            }),
            &DrawOptions::new(),
        );

        window
            .update_with_buffer(dt.get_data(), size.0, size.1)
            .unwrap();
    }
    Ok(None)
}

fn pong_process(
    window: &mut Window,
    listener: &NetListener,
    initial_state: BallBoard,
) -> io::Result<bool> {
    let size = window.get_size();
    let mut pong = Pong::new(
        initial_state.ball_x,
        initial_state.ball_y,
        initial_state.ball_speed_x,
        initial_state.ball_speed_y,
        initial_state.board_row,
    );
    let mut dt = DrawTarget::new(size.0 as i32, size.1 as i32);
    let mut last_time = SystemTime::now();
    let mut send_pong = None;
    let mut last_received_x = 0;
    while window.is_open() && !window.is_key_down(Key::Escape) {
        let now = SystemTime::now();
        let duration = now.duration_since(last_time).unwrap();
        if window.is_key_pressed(Key::Down, KeyRepeat::No) {
            pong.down_pressed()
        }
        if window.is_key_pressed(Key::Up, KeyRepeat::No) {
            pong.up_pressed()
        }
        if window.is_key_released(Key::Down) {
            pong.down_released()
        }
        if window.is_key_released(Key::Up) {
            pong.up_released()
        }

        match pong.step(duration) {
            StepResult::Proceed => (),
            StepResult::Pong {
                ball_x,
                ball_y,
                ball_speed_y,
            } => {
                send_pong = Some(ball_speed_y);
                println!("send pong {} {} {}", ball_x, ball_y, ball_speed_y);
                listener.send_pong(ball_x, ball_y, ball_speed_y)?;
            }
            StepResult::Board(row) => {
                println!("send board");
                listener.send_board(row)?
            }
            StepResult::Loose => return Ok(false),
        };
        dt.clear(SolidSource::from_unpremultiplied_argb(0xff, 0x0, 0x0, 0x0));
        pong.draw(&mut dt);
        window
            .update_with_buffer(dt.get_data(), size.0, size.1)
            .unwrap();
        last_time = now;

        match listener.get_message().unwrap() {
            None => (),
            Some(NetMessage::BallBoard {
                ball_x,
                ball_y,
                ball_speed_x,
                ball_speed_y,
                board_row,
            }) => {
                pong.set_left_board(board_row);
                if last_received_x < ball_x && send_pong.is_some() {
                    let ball_speed_y = send_pong.unwrap();
                    let (x, y) = pong.get_ball();
                    println!("send pong {} {} {}", x, y, ball_speed_y);
                    listener.send_pong(x, y, ball_speed_y)?;
                } else {
                    send_pong = None;
                    pong.set_ball(ball_x, ball_y, ball_speed_x, ball_speed_y, now);
                }
                last_received_x = ball_x;
            }
            Some(NetMessage::Score { .. }) => return Ok(false),
            _ => todo!(),
        }
    }
    Ok(true)
}

fn main() {
    let mut window = Window::new(
        "LAN Pong",
        WIDTH,
        HEIGHT,
        WindowOptions {
            ..WindowOptions::default()
        },
    )
    .unwrap();
    window.limit_update_rate(Some(std::time::Duration::from_micros(16600)));

    let listener = NetListener::new().unwrap();

    let mut state = AppState::Lobby;
    loop {
        match state {
            AppState::Lobby => {
                if lobby_process(&mut window, &listener).unwrap() {
                    break;
                } else {
                    state = AppState::Score;
                }
            }
            AppState::Score => match score_process(&mut window, &listener).unwrap() {
                Some(ballboard) => state = AppState::Game(ballboard),
                None => break,
            },
            AppState::Game(ballboard) => {
                if pong_process(&mut window, &listener, ballboard).unwrap() {
                    break;
                } else {
                    state = AppState::Score;
                }
            }
        };
    }
}
