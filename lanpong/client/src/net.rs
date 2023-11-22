use std::{io, net};

use num_traits::FromPrimitive;

const UDP_PORT: u16 = 7657;
const MAGIC_COOKIE: &[u8] = b"PONG";

#[derive(Debug)]
pub enum NetMessage {
    Announce,
    Join,
    Ready,
    Board {
        col: u8,
    },
    BallBoard {
        ball_x: i16,
        ball_y: i16,
        ball_speed_x: i16,
        ball_speed_y: i16,
        board_row: u8,
    },
    Score {
        left: u16,
        right: u16,
    },
}

pub struct NetListener {
    socket: net::UdpSocket,
}

#[derive(Clone, Copy, Debug, FromPrimitive)]
enum NetMessageTag {
    Announce = 0,  // broadcast announcement
    Join = 1,      // join game
    Ready = 2,     // player is ready
    Pong = 3,      // bounce right
    Board = 4,     // board state
    BallBoard = 5, // ball and board state
    Score = 6,
}

/*

   1. Server sends Announce packets
   2. Client recvs Announce and send Join
   3. Server and client wait for player input
   4. As soon as player hits enter, Ready is sent
   5. When both are ready, game mode is started
   6. In game mode, Ball, Board and BallBoard packets are sent
   7. As soon as someone scores, Score is sent.
   8. The other side sends back Score to confirm. Go to step 3.

*/

impl NetListener {
    pub fn new() -> io::Result<Self> {
        let socket = net::UdpSocket::bind(("0.0.0.0", UDP_PORT))?;
        socket.set_nonblocking(true)?;
        Ok(Self { socket })
    }

    pub fn get_message(&self) -> io::Result<Option<NetMessage>> {
        let mut buf = [0; 64];
        let (msg, addr) = match self.socket.recv_from(&mut buf) {
            Ok((size, addr)) => (parse_packet(&buf[..size]), addr),
            Err(e) if e.kind() == io::ErrorKind::WouldBlock => return Ok(None),
            Err(e) => return Err(e),
        };
        println!("got message {:?} from {}", msg, addr);
        if let Some(NetMessage::Announce) = msg {
            self.socket.connect(addr)?;
        }
        Ok(msg)
    }

    pub fn send_join(&self) -> io::Result<()> {
        let mut packet = [0; MAGIC_COOKIE.len() + 1];
        init_packet(&mut packet, NetMessageTag::Join);
        let len = self.socket.send(&packet)?;
        assert_eq!(len, packet.len());
        Ok(())
    }

    pub fn send_ready(&self) -> io::Result<()> {
        let mut packet = [0; MAGIC_COOKIE.len() + 1];
        init_packet(&mut packet, NetMessageTag::Ready);
        let len = self.socket.send(&packet)?;
        assert_eq!(len, packet.len());
        Ok(())
    }

    pub fn send_pong(&self, ball_x: i16, ball_y: i16, ball_speed_y: i16) -> io::Result<()> {
        let mut packet = [0; MAGIC_COOKIE.len() + 1 + 6];
        let i = init_packet(&mut packet, NetMessageTag::Pong);
        packet[i + 0..i + 2].copy_from_slice(&ball_x.to_le_bytes());
        packet[i + 2..i + 4].copy_from_slice(&ball_y.to_le_bytes());
        packet[i + 4..i + 6].copy_from_slice(&ball_speed_y.to_le_bytes());
        let len = self.socket.send(&packet)?;
        assert_eq!(len, packet.len());
        Ok(())
    }

    pub fn send_board(&self, board_row: u8) -> io::Result<()> {
        let mut packet = [0; MAGIC_COOKIE.len() + 1 + 1];
        let i = init_packet(&mut packet, NetMessageTag::Board);
        packet[i] = board_row;
        let len = self.socket.send(&packet)?;
        assert_eq!(len, packet.len());
        Ok(())
    }
}

fn init_packet(packet: &mut [u8], tag: NetMessageTag) -> usize {
    packet[0..MAGIC_COOKIE.len()].copy_from_slice(MAGIC_COOKIE);
    packet[MAGIC_COOKIE.len()] = tag as u8;
    return MAGIC_COOKIE.len() + 1;
}

fn parse_packet(packet: &[u8]) -> Option<NetMessage> {
    let mut i = 0;
    if &packet[0..MAGIC_COOKIE.len()] != MAGIC_COOKIE {
        println!("bad cookie {:?}", &packet[0..MAGIC_COOKIE.len()]);
        return None;
    }
    i += MAGIC_COOKIE.len();

    if i >= packet.len() {
        println!("too short {}", packet.len());
        return None;
    }
    let tag: NetMessageTag = FromPrimitive::from_u8(packet[i])?;
    i += 1;
    match tag {
        NetMessageTag::Announce => Some(NetMessage::Announce),
        NetMessageTag::Join => Some(NetMessage::Join),
        NetMessageTag::Ready => Some(NetMessage::Ready),
        NetMessageTag::Pong => unreachable!(),
        NetMessageTag::BallBoard => {
            if i + 9 != packet.len() {
                return None;
            }
            let mut x = [0; 2];
            let mut y = [0; 2];
            let mut sx = [0; 2];
            let mut sy = [0; 2];
            x.copy_from_slice(&packet[i..i + 2]);
            y.copy_from_slice(&packet[i + 2..i + 4]);
            sx.copy_from_slice(&packet[i + 4..i + 6]);
            sy.copy_from_slice(&packet[i + 6..i + 8]);
            let x = i16::from_le_bytes(x);
            let y = i16::from_le_bytes(y);
            let sx = i16::from_le_bytes(sx);
            let sy = i16::from_le_bytes(sy);
            Some(NetMessage::BallBoard {
                ball_x: x,
                ball_y: y,
                ball_speed_x: sx,
                ball_speed_y: sy,
                board_row: packet[i + 8],
            })
        }
        NetMessageTag::Board => {
            if i + 1 != packet.len() {
                return None;
            }
            Some(NetMessage::Board { col: packet[i] })
        }
        NetMessageTag::Score => {
            if i + 4 != packet.len() {
                return None;
            }
            let mut l = [0; 2];
            let mut r = [0; 2];
            l.copy_from_slice(&packet[i..i + 2]);
            r.copy_from_slice(&packet[i + 2..i + 4]);
            let l = u16::from_le_bytes(l);
            let r = u16::from_le_bytes(r);
            Some(NetMessage::Score { left: l, right: r })
        }
    }
}
