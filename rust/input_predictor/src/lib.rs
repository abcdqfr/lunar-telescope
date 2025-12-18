//! Input Predictor - Rust Performance-Critical Module
//!
//! This module provides high-performance input prediction algorithms
//! for pointer motion and scroll events. It is exposed via a C ABI
//! for integration with the C core.

use libc::{c_double, c_int, c_uint, c_ulonglong, size_t};

/// Input event types (must match C enum)
#[repr(C)]
pub enum InputEventType {
    PointerMotion = 0,
    PointerButton = 1,
    Scroll = 2,
    Key = 3,
    Touch = 4,
}

/// Prediction parameters
#[repr(C)]
pub struct PredictionParams {
    pub window_ms: c_uint,
    pub smoothing_factor: c_double,
    pub velocity_decay: c_double,
}

/// Velocity tracker for motion prediction
pub struct VelocityTracker {
    samples: Vec<(f64, f64, f64)>, // (timestamp, dx, dy)
    max_samples: usize,
}

impl VelocityTracker {
    pub fn new(max_samples: usize) -> Self {
        Self {
            samples: Vec::with_capacity(max_samples),
            max_samples,
        }
    }

    pub fn add_sample(&mut self, timestamp: f64, dx: f64, dy: f64) {
        self.samples.push((timestamp, dx, dy));
        if self.samples.len() > self.max_samples {
            self.samples.remove(0);
        }
    }

    pub fn predict(&self, prediction_window_ms: u32) -> (f64, f64) {
        if self.samples.len() < 2 {
            return (0.0, 0.0);
        }

        // Calculate velocity from recent samples
        let window_ms = prediction_window_ms as f64;
        let now = self.samples.last().unwrap().0;
        let cutoff = now - (window_ms / 1000.0);

        let mut total_dx = 0.0;
        let mut total_dy = 0.0;
        let mut total_dt = 0.0;

        for i in 1..self.samples.len() {
            let (t1, x1, y1) = self.samples[i - 1];
            let (t2, x2, y2) = self.samples[i];

            if t1 < cutoff {
                continue;
            }

            let dt = t2 - t1;
            if dt > 0.0 {
                total_dx += (x2 - x1) / dt;
                total_dy += (y2 - y1) / dt;
                total_dt += dt;
            }
        }

        if total_dt > 0.0 {
            let avg_vx = total_dx / (self.samples.len() - 1) as f64;
            let avg_vy = total_dy / (self.samples.len() - 1) as f64;

            // Predict future position
            let prediction_dt = window_ms / 1000.0;
            (avg_vx * prediction_dt, avg_vy * prediction_dt)
        } else {
            (0.0, 0.0)
        }
    }

    pub fn clear(&mut self) {
        self.samples.clear();
    }
}

/// Input predictor state
pub struct InputPredictor {
    pointer_tracker: VelocityTracker,
    scroll_tracker: VelocityTracker,
    params: PredictionParams,
}

impl InputPredictor {
    pub fn new(params: PredictionParams) -> Self {
        Self {
            pointer_tracker: VelocityTracker::new(10),
            scroll_tracker: VelocityTracker::new(10),
            params,
        }
    }

    pub fn predict_pointer_motion(
        &mut self,
        timestamp: f64,
        dx: f64,
        dy: f64,
    ) -> (f64, f64) {
        self.pointer_tracker.add_sample(timestamp, dx, dy);
        self.pointer_tracker.predict(self.params.window_ms)
    }

    pub fn predict_scroll(
        &mut self,
        timestamp: f64,
        dx: f64,
        dy: f64,
    ) -> (f64, f64) {
        self.scroll_tracker.add_sample(timestamp, dx, dy);
        self.scroll_tracker.predict(self.params.window_ms)
    }

    pub fn reset(&mut self) {
        self.pointer_tracker.clear();
        self.scroll_tracker.clear();
    }
}

// C ABI exports - match rust_predictor.h interface

#[no_mangle]
pub extern "C" fn rust_input_predictor_create(
    window_ms: c_uint,
    smoothing_factor: c_double,
    velocity_decay: c_double,
) -> *mut InputPredictor {
    let params = PredictionParams {
        window_ms,
        smoothing_factor,
        velocity_decay,
    };
    Box::into_raw(Box::new(InputPredictor::new(params)))
}

#[no_mangle]
pub extern "C" fn rust_input_predictor_destroy(predictor: *mut InputPredictor) {
    if !predictor.is_null() {
        unsafe {
            drop(Box::from_raw(predictor));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_input_predictor_predict_pointer(
    predictor: *mut InputPredictor,
    timestamp: c_double,
    dx: c_double,
    dy: c_double,
    predicted_dx: *mut c_double,
    predicted_dy: *mut c_double,
) -> c_int {
    if predictor.is_null() || predicted_dx.is_null() || predicted_dy.is_null() {
        return -1;
    }

    unsafe {
        let predictor = &mut *predictor;
        let (pdx, pdy) = predictor.predict_pointer_motion(timestamp, dx, dy);
        *predicted_dx = pdx;
        *predicted_dy = pdy;
    }

    0
}

#[no_mangle]
pub extern "C" fn rust_input_predictor_predict_scroll(
    predictor: *mut InputPredictor,
    timestamp: c_double,
    dx: c_double,
    dy: c_double,
    predicted_dx: *mut c_double,
    predicted_dy: *mut c_double,
) -> c_int {
    if predictor.is_null() || predicted_dx.is_null() || predicted_dy.is_null() {
        return -1;
    }

    unsafe {
        let predictor = &mut *predictor;
        let (pdx, pdy) = predictor.predict_scroll(timestamp, dx, dy);
        *predicted_dx = pdx;
        *predicted_dy = pdy;
    }

    0
}

#[no_mangle]
pub extern "C" fn rust_input_predictor_reset(predictor: *mut InputPredictor) -> c_int {
    if predictor.is_null() {
        return -1;
    }

    unsafe {
        (*predictor).reset();
    }

    0
}

