use crate::PIXEL_PER_SQUARE;
use geometric::Dot;
use geometric::Vector2;
use raqote::DrawOptions;
use raqote::SolidSource;
use raqote::Source;
use raqote::{DrawTarget, PathBuilder};
use std::cmp::{max, min};
use std::time;
use std::time::SystemTime;

pub struct Pong {
    ball_x: f32, // col * 256
    ball_y: f32, // row * 512
    board_left: Board,
    board_right: Board,
    ball_origin: Option<BallState>,
    ball_speed: Option<Vector2<f32>>,
    speed_factor: Option<f32>,
}

pub enum StepResult {
    Proceed,
    Loose,
    Pong {
        ball_x: i16,
        ball_y: i16,
        ball_speed_y: i16,
    },
    Board(u8),
}

struct Board {
    col: u8,
    y: u16,
    speed: i16,
    acceleration: i16,
}

struct BallState {
    x: f32,
    y: f32,
    t: SystemTime,
}

const TOP: u16 = 1 * 512;
const BOTTOM: u16 = 30 * 512;
const LEFT: u16 = 1 * 256;
const RIGHT: u16 = 79 * 256;

const VGA_ROWS: u8 = 30;
const VGA_COLS: u8 = 80;

const BOARD_SIZE: u8 = 3;

const BOARD_ACCELERATION: i16 = 1;

const TIME_FACTOR: f32 = 1.0;

impl Pong {
    pub fn new(ball_x: i16, ball_y: i16, _ball_speed_x: i16, _ball_speed_y: i16, board_left: u8) -> Self {
        Self {
            ball_x: ball_x as f32,
            ball_y: ball_y as f32,
            board_left: Board::new(0, Some(board_left)),
            board_right: Board::new(VGA_COLS - 1, None),
            ball_origin: None,
            ball_speed: None,
            speed_factor: None
        }
    }

    pub fn draw(&self, dt: &mut DrawTarget) {
        self.board_left.draw(dt);
        self.board_right.draw(dt);
        self.draw_ball(dt);
    }

    pub fn up_pressed(&mut self) {
        self.board_right.start_up();
    }

    pub fn up_released(&mut self) {
        self.board_right.stop();
    }

    pub fn down_pressed(&mut self) {
        self.board_right.start_down();
    }

    pub fn down_released(&mut self) {
        self.board_right.stop();
    }

    pub fn get_ball(&self) -> (i16, i16) {
        (self.ball_x as i16, self.ball_y as i16)
    }

    pub fn step(&mut self, dt: time::Duration) -> StepResult {
        let board_update = self.board_right.step(dt);

        let dt_ms = dt.as_millis() as f32;
        let top = TOP as f32;
        let bot = BOTTOM as f32;
        let left = LEFT as f32;
        let right = RIGHT as f32;
        if let Some(ball_speed) = self.ball_speed.as_mut() {
            self.ball_y += ball_speed.y() * dt_ms;
            if self.ball_y < top {
                bounce(&mut self.ball_y, top);
                self.ball_origin
                    .as_mut()
                    .map(|origin| bounce(&mut origin.y, top));
                ball_speed.components[1] = -ball_speed.components[1];
            }
            if self.ball_y >= bot {
                bounce(&mut self.ball_y, bot);
                self.ball_origin
                    .as_mut()
                    .map(|origin| bounce(&mut origin.y, bot));
                ball_speed.components[1] = -ball_speed.components[1];
            }
            self.ball_x += ball_speed.x() * dt_ms;
            if self.ball_x < left {
                bounce(&mut self.ball_x, left);
                self.ball_origin
                    .as_mut()
                    .map(|origin| bounce(&mut origin.x, left));
            }
            if self.ball_x >= right {
                let y_sq = (self.ball_y / 256.0) as u8;
                println!(
                    "right collision x={} y_sq={} y_board={}",
                    self.ball_x, y_sq, self.board_right.y
                );
                if let Some(y_speed) = self.board_right.get_hit(y_sq) {
                    bounce(&mut self.ball_x, right);
                    self.ball_origin = Some(BallState {
                        x: self.ball_x,
                        y: self.ball_y,
                        t: SystemTime::now(),
                    });
                    ball_speed.components[0] = -ball_speed.components[0] + 1.0 * TIME_FACTOR;
                    ball_speed.components[1] = y_speed as f32 * TIME_FACTOR;
                    return StepResult::Pong {
                        ball_x: self.ball_x as i16,
                        ball_y: self.ball_y as i16,
                        ball_speed_y: y_speed,
                    };
                } else {
                    return StepResult::Loose;
                }
            }
            if let Some(y) = board_update {
                StepResult::Board(y)
            } else {
                StepResult::Proceed
            }
        } else {
            StepResult::Proceed
        }
    }

    pub fn set_ball(&mut self, x: i16, y: i16, sx: i16, sy: i16, now: SystemTime) {
        self.ball_x = x as f32;
        self.ball_y = y as f32;

        let got_speed = Vector2::new([sx as f32, sy as f32]);

        if let Some(old_speed) = self.ball_speed {
            if got_speed.x() * old_speed.x() < 0.0 {
                // Ball has changed horizontal position - accept new speed
                if let Some(speed_factor) = self.speed_factor {
                    self.ball_speed = Some(got_speed * speed_factor)
                } else {
                    self.ball_speed = None;
                }
                return;
            }
        }

        if let Some(last_origin) = self.ball_origin.take() {
            let dt = now.duration_since(last_origin.t).unwrap().as_millis() as f32;
            if dt < 2000.0 {
                let dx_straight =
                    Vector2::new([self.ball_x - last_origin.x, self.ball_y - last_origin.y]);
                let top = TOP as f32;
                let bottom = BOTTOM as f32;
                let dx_bounce_top = Vector2::new([
                    self.ball_x - last_origin.x,
                    self.ball_y - 2.0 * top + last_origin.y,
                ]);
                let dx_bounce_bot = Vector2::new([
                    self.ball_x - last_origin.x,
                    self.ball_y - 2.0 * bottom * last_origin.y,
                ]);
                let candidate_speeds = [dx_straight / dt, dx_bounce_top / dt, dx_bounce_bot / dt];
                let got_speed_norm = got_speed.norm();
                let best_speed_i = candidate_speeds
                    .iter()
                    .map(|new_speed| new_speed.norm().dot(got_speed_norm))
                    .enumerate()
                    .max_by(|(_, x), (_, y)| x.total_cmp(y))
                    .unwrap()
                    .0;
                match best_speed_i {
                    0 => println!("BEST: straight"),
                    1 => println!("BEST: bounce from top"),
                    2 => println!("BEST: bounce from bottom"),
                    _ => unreachable!(),
                };
                let best_speed = candidate_speeds[best_speed_i];
                let speed_factor_x = best_speed.x() / got_speed.x();
                let speed_factor_y = best_speed.y() / got_speed.y();
                let speed_factor = (speed_factor_x + speed_factor_y) / 2.0;
                println!("best_speed = {:?}, speed_factor = {}", best_speed, speed_factor);
                self.speed_factor = Some(speed_factor);
                self.ball_speed = Some(best_speed);
            } else {
                self.ball_speed = None;
            }
        }
        self.ball_origin = Some(BallState {
            x: self.ball_x,
            y: self.ball_y,
            t: now,
        });
    }

    pub fn set_left_board(&mut self, row: u8) {
        self.board_left.set_row(row);
    }

    fn draw_ball(&self, dt: &mut DrawTarget) {
        let mut pb = PathBuilder::new();
        let y_sq = (self.ball_y / 256.0) as u8;
        let x_sq = (self.ball_x / 256.0) as u8;
        pb.rect(
            qp_to_pixel(x_sq),
            qp_to_pixel(y_sq),
            qp_to_pixel(1),
            qp_to_pixel(1),
        );
        let path = pb.finish();
        dt.fill(
            &path,
            &Source::Solid(SolidSource::from_unpremultiplied_argb(
                0xff, 0xff, 0xff, 0xff,
            )),
            &DrawOptions::new(),
        );
    }
}

fn bounce(x: &mut f32, b: f32) {
    *x = 2.0 * b - *x;
}

impl Board {
    fn new(col: u8, row: Option<u8>) -> Self {
        Self {
            y: (row.unwrap_or(VGA_ROWS / 2) as u16) << 8,
            col,
            acceleration: 0,
            speed: 0,
        }
    }

    fn set_row(&mut self, row: u8) {
        self.y = (row as u16) << 8;
    }

    fn get_hit(&self, row: u8) -> Option<i16> {
        let y_sq = (self.y / 128) as u8;
        if row >= y_sq - 2 * BOARD_SIZE && row <= y_sq + 2 * BOARD_SIZE {
            Some((row as i16) - (y_sq as i16))
        } else {
            None
        }
    }

    fn step(&mut self, dt: time::Duration) -> Option<u8> {
        let acceleration = self.acceleration as f32 * TIME_FACTOR * (dt.as_millis() as f32);
        let new_speed = self.speed as f32 + acceleration;
        self.speed = new_speed as i16;
        let last_y = self.y / 256;
        self.y = (self.y as i16 + self.speed) as u16;
        self.y = max(self.y, ((BOARD_SIZE as u16) << 8) + (TOP >> 1));
        self.y = min(self.y, (BOTTOM >> 1) - ((BOARD_SIZE as u16) << 8) - 1);
        if last_y != self.y / 256 {
            Some((self.y / 256) as u8)
        } else {
            None
        }
    }

    fn stop(&mut self) {
        self.speed = 0;
        self.acceleration = 0;
    }

    fn start_down(&mut self) {
        self.acceleration = BOARD_ACCELERATION;
    }

    fn start_up(&mut self) {
        self.acceleration = -BOARD_ACCELERATION;
    }

    fn draw(&self, dt: &mut DrawTarget) {
        let mut pb = PathBuilder::new();
        let y_sq = (self.y / 128) as u8;
        let top = y_sq - BOARD_SIZE * 2;
        pb.rect(
            qp_to_pixel(self.col),
            qp_to_pixel(top),
            qp_to_pixel(1),
            qp_to_pixel(4 * BOARD_SIZE),
        );
        let path = pb.finish();
        dt.fill(
            &path,
            &Source::Solid(SolidSource::from_unpremultiplied_argb(
                0xff, 0xff, 0xff, 0xff,
            )),
            &DrawOptions::new(),
        );
    }
}

fn qp_to_pixel(qp: u8) -> f32 {
    (qp as usize * PIXEL_PER_SQUARE) as f32
}
