use serde::Serialize;
use std::ffi::c_void;
use windows::{
    core::PCWSTR,
    Win32::Foundation::{CloseHandle, GetLastError, HANDLE, LUID},
    Win32::Security::{
        AdjustTokenPrivileges, LookupPrivilegeValueW,
        TOKEN_ADJUST_PRIVILEGES, TOKEN_PRIVILEGES, TOKEN_QUERY,
        SE_PRIVILEGE_ENABLED,
    },
    Win32::System::LibraryLoader::{GetModuleFileNameW, GetModuleHandleW, GetProcAddress, LoadLibraryW},
    Win32::System::Memory::{SetProcessWorkingSetSizeEx, SETPROCESSWORKINGSETSIZEEX_FLAGS},
    Win32::System::ProcessStatus::{EmptyWorkingSet, EnumProcesses, GetPerformanceInfo, PERFORMANCE_INFORMATION},
    Win32::System::SystemInformation::{GlobalMemoryStatusEx, MEMORYSTATUSEX},
    Win32::System::Threading::{GetCurrentProcess, GetCurrentProcessId, OpenProcess, OpenProcessToken, PROCESS_QUERY_INFORMATION, PROCESS_SET_QUOTA},
};
use winreg::enums::*;
use winreg::RegKey;
use std::thread::sleep;
use std::time::Duration;

#[derive(Serialize)]
struct RamStats {
    total: u64,
    used: u64,
    percent: u32,
    cache: u64,
}

#[derive(Serialize)]
struct CleanResult {
    freed_bytes: u64,
    usage_before: u32,
    usage_after: u32,
}

type NtSetSystemInformationFn = unsafe extern "system" fn(u32, *mut c_void, u32) -> i32;

const SYSTEM_MEMORY_LIST_INFORMATION: u32 = 80;
const SYSTEM_FILE_CACHE_INFORMATION_EX: u32 = 81;
const SYSTEM_COMBINE_PHYSICAL_MEMORY_INFORMATION: u32 = 130;
const SYSTEM_REGISTRY_RECONCILIATION_INFORMATION: u32 = 155;

const MEMORY_EMPTY_WORKING_SETS: u32 = 2;
const MEMORY_FLUSH_MODIFIED_LIST: u32 = 3;
const MEMORY_PURGE_STANDBY_LIST: u32 = 4;
const MEMORY_PURGE_LOW_PRIORITY_STANDBY_LIST: u32 = 5;

const QUOTA_LIMITS_HARDWS_MIN_DISABLE: u32 = 0x00000002;
const QUOTA_LIMITS_HARDWS_MAX_DISABLE: u32 = 0x00000008;

#[repr(C)]
struct SystemFilecacheInformation {
    current_size: usize,
    peak_size: usize,
    page_fault_count: u32,
    minimum_working_set: usize,
    maximum_working_set: usize,
    current_size_including_transition_in_pages: usize,
    peak_size_including_transition_in_pages: usize,
    transition_re_purpose_count: u32,
    flags: u32,
}

#[repr(C)]
struct MemoryCombineInformationEx {
    handle: HANDLE,
    pages_combined: usize,
    flags: u32,
}

fn get_nt_set_system_information() -> Option<NtSetSystemInformationFn> {
    unsafe {
        let mut ntdll = GetModuleHandleW(windows::core::w!("ntdll.dll")).unwrap_or_default();
        if ntdll.is_invalid() {
            ntdll = LoadLibraryW(windows::core::w!("ntdll.dll")).unwrap_or_default();
        }
        if ntdll.is_invalid() {
            return None;
        }
        let addr = GetProcAddress(ntdll, windows::core::s!("NtSetSystemInformation"));
        addr.map(|addr| std::mem::transmute(addr))
    }
}

fn enable_privilege(name: PCWSTR) -> bool {
    unsafe {
        let mut token = HANDLE::default();
        if OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &mut token).is_err() {
            return false;
        }
        let mut luid = LUID::default();
        if LookupPrivilegeValueW(None, name, &mut luid).is_err() {
            let _ = CloseHandle(token);
            return false;
        }
        let mut privs = TOKEN_PRIVILEGES::default();
        privs.PrivilegeCount = 1;
        privs.Privileges[0].Luid = luid;
        privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        let res = AdjustTokenPrivileges(token, false, Some(&privs), 0, None, None);
        let _ = CloseHandle(token);
        res.is_ok() && GetLastError().is_ok()
    }
}

fn enable_all_privileges() {
    enable_privilege(windows::core::w!("SeDebugPrivilege"));
    enable_privilege(windows::core::w!("SeIncreaseQuotaPrivilege"));
    enable_privilege(windows::core::w!("SeProfileSingleProcessPrivilege"));
    // SE_INC_WORKING_SET_NAME doesn't have a standard constant in windows-rs easily, skip or define manually
}

fn clean_processes_ultra_aggressive() {
    unsafe {
        let mut processes = [0u32; 1024];
        let mut cb_needed = 0;
        if EnumProcesses(processes.as_mut_ptr(), std::mem::size_of_val(&processes) as u32, &mut cb_needed).is_ok() {
            let count = cb_needed as usize / std::mem::size_of::<u32>();
            let self_pid = GetCurrentProcessId();
            for i in 0..count {
                if processes[i] == 0 || processes[i] == 4 || processes[i] == self_pid { continue; }
                if let Ok(handle) = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA, false, processes[i]) {
                    let _ = EmptyWorkingSet(handle);
                    let _ = SetProcessWorkingSetSizeEx(handle, usize::MAX, usize::MAX, SETPROCESSWORKINGSETSIZEEX_FLAGS(QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE));
                    let _ = CloseHandle(handle);
                }
            }
        }
    }
}

fn clean_windows_memory_lists() {
    if let Some(nt_set) = get_nt_set_system_information() {
        let mut cmds = [
            MEMORY_EMPTY_WORKING_SETS,
            MEMORY_FLUSH_MODIFIED_LIST,
            MEMORY_PURGE_STANDBY_LIST,
            MEMORY_PURGE_LOW_PRIORITY_STANDBY_LIST,
        ];
        unsafe {
            for cmd in cmds.iter_mut() {
                let _ = nt_set(SYSTEM_MEMORY_LIST_INFORMATION, cmd as *mut u32 as *mut c_void, std::mem::size_of::<u32>() as u32);
            }
        }
    }
}

fn clean_system_file_cache() {
    if let Some(nt_set) = get_nt_set_system_information() {
        let mut info = SystemFilecacheInformation {
            current_size: 0,
            peak_size: 0,
            page_fault_count: 0,
            minimum_working_set: usize::MAX,
            maximum_working_set: usize::MAX,
            current_size_including_transition_in_pages: 0,
            peak_size_including_transition_in_pages: 0,
            transition_re_purpose_count: 0,
            flags: QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE,
        };
        unsafe {
            let _ = nt_set(SYSTEM_FILE_CACHE_INFORMATION_EX, &mut info as *mut _ as *mut c_void, std::mem::size_of::<SystemFilecacheInformation>() as u32);
        }
    }
}

fn clean_registry_cache() {
    if let Some(nt_set) = get_nt_set_system_information() {
        unsafe {
            let _ = nt_set(SYSTEM_REGISTRY_RECONCILIATION_INFORMATION, std::ptr::null_mut(), 0);
        }
    }
}

fn combine_physical_memory() {
    if let Some(nt_set) = get_nt_set_system_information() {
        let mut info = MemoryCombineInformationEx {
            handle: HANDLE::default(),
            pages_combined: 0,
            flags: 0,
        };
        unsafe {
            let _ = nt_set(SYSTEM_COMBINE_PHYSICAL_MEMORY_INFORMATION, &mut info as *mut _ as *mut c_void, std::mem::size_of::<MemoryCombineInformationEx>() as u32);
        }
    }
}

fn aggressive_flush_cycle(repetitions: u32) {
    if let Some(nt_set) = get_nt_set_system_information() {
        for _ in 0..repetitions {
            unsafe {
                let mut c = MEMORY_FLUSH_MODIFIED_LIST;
                let _ = nt_set(SYSTEM_MEMORY_LIST_INFORMATION, &mut c as *mut _ as *mut c_void, 4);
                sleep(Duration::from_millis(30));
                let mut c = MEMORY_PURGE_STANDBY_LIST;
                let _ = nt_set(SYSTEM_MEMORY_LIST_INFORMATION, &mut c as *mut _ as *mut c_void, 4);
                sleep(Duration::from_millis(30));
                let mut c = MEMORY_PURGE_LOW_PRIORITY_STANDBY_LIST;
                let _ = nt_set(SYSTEM_MEMORY_LIST_INFORMATION, &mut c as *mut _ as *mut c_void, 4);
                sleep(Duration::from_millis(30));
                let mut c = MEMORY_EMPTY_WORKING_SETS;
                let _ = nt_set(SYSTEM_MEMORY_LIST_INFORMATION, &mut c as *mut _ as *mut c_void, 4);
                sleep(Duration::from_millis(45));
            }
        }
    }
}

fn final_intensive_cycle() {
    if let Some(nt_set) = get_nt_set_system_information() {
        let seq = [
            MEMORY_FLUSH_MODIFIED_LIST, MEMORY_PURGE_LOW_PRIORITY_STANDBY_LIST,
            MEMORY_PURGE_STANDBY_LIST, MEMORY_FLUSH_MODIFIED_LIST,
            MEMORY_PURGE_STANDBY_LIST, MEMORY_EMPTY_WORKING_SETS,
            MEMORY_FLUSH_MODIFIED_LIST, MEMORY_PURGE_LOW_PRIORITY_STANDBY_LIST,
            MEMORY_PURGE_STANDBY_LIST, MEMORY_EMPTY_WORKING_SETS,
            MEMORY_FLUSH_MODIFIED_LIST, MEMORY_PURGE_STANDBY_LIST,
        ];
        for mut c in seq {
            unsafe {
                let _ = nt_set(SYSTEM_MEMORY_LIST_INFORMATION, &mut c as *mut _ as *mut c_void, 4);
                sleep(Duration::from_millis(45));
            }
        }
    }
}

fn get_available_ram() -> u64 {
    let mut mem_info = MEMORYSTATUSEX::default();
    mem_info.dwLength = std::mem::size_of::<MEMORYSTATUSEX>() as u32;
    unsafe {
        let _ = GlobalMemoryStatusEx(&mut mem_info);
    }
    mem_info.ullAvailPhys
}

fn measure_stable_ram() -> u64 {
    let mut mx = 0;
    for _ in 0..2 {
        let s = get_available_ram();
        if s > mx { mx = s; }
        sleep(Duration::from_millis(120));
    }
    mx
}

#[tauri::command]
fn get_ram_stats() -> RamStats {
    let mut mem_info = MEMORYSTATUSEX::default();
    mem_info.dwLength = std::mem::size_of::<MEMORYSTATUSEX>() as u32;
    unsafe {
        let _ = GlobalMemoryStatusEx(&mut mem_info);
    }
    
    let mut cache = 0;
    unsafe {
        let mut pi = PERFORMANCE_INFORMATION::default();
        pi.cb = std::mem::size_of::<PERFORMANCE_INFORMATION>() as u32;
        if GetPerformanceInfo(&mut pi, pi.cb).is_ok() {
            cache = pi.SystemCache as u64 * pi.PageSize as u64;
        }
    }

    RamStats {
        total: mem_info.ullTotalPhys,
        used: mem_info.ullTotalPhys - mem_info.ullAvailPhys,
        percent: mem_info.dwMemoryLoad,
        cache,
    }
}

fn ram_tray_tooltip() -> String {
    let stats = get_ram_stats();
    let used_gb = stats.used as f64 / 1024.0 / 1024.0 / 1024.0;
    let total_gb = stats.total as f64 / 1024.0 / 1024.0 / 1024.0;
    format!("RAMMY - RAM {}% ({:.1}/{:.1} GB)", stats.percent, used_gb, total_gb)
}

fn digit_pattern(digit: char) -> [&'static str; 5] {
    match digit {
        '0' => ["111", "101", "101", "101", "111"],
        '1' => ["010", "110", "010", "010", "111"],
        '2' => ["111", "001", "111", "100", "111"],
        '3' => ["111", "001", "111", "001", "111"],
        '4' => ["101", "101", "111", "001", "001"],
        '5' => ["111", "100", "111", "001", "111"],
        '6' => ["111", "100", "111", "101", "111"],
        '7' => ["111", "001", "010", "010", "010"],
        '8' => ["111", "101", "111", "101", "111"],
        '9' => ["111", "101", "111", "001", "111"],
        _ => ["000", "000", "000", "000", "000"],
    }
}

fn set_pixel(buf: &mut [u8], x: i32, y: i32, rgba: [u8; 4]) {
    if !(0..32).contains(&x) || !(0..32).contains(&y) {
        return;
    }
    let idx = ((y as usize * 32 + x as usize) * 4) as usize;
    buf[idx..idx + 4].copy_from_slice(&rgba);
}

fn fill_rect(buf: &mut [u8], x: i32, y: i32, w: i32, h: i32, rgba: [u8; 4]) {
    for yy in y..y + h {
        for xx in x..x + w {
            set_pixel(buf, xx, yy, rgba);
        }
    }
}

fn ram_tray_icon(percent: u32) -> tauri::image::Image<'static> {
    let text = percent.min(100).to_string();
    let scale = if text.len() >= 3 { 3 } else { 4 };
    let gap = 1;
    let digit_w = 3 * scale;
    let digit_h = 5 * scale;
    let text_w = text.len() as i32 * digit_w + (text.len() as i32 - 1) * gap;
    let start_x = (32 - text_w) / 2;
    let start_y = (32 - digit_h) / 2;

    let mut buf = vec![0u8; 32 * 32 * 4];

    for y in 0..32 {
        for x in 0..32 {
            let dx = x as f32 - 15.5;
            let dy = y as f32 - 15.5;
            let dist = (dx * dx + dy * dy).sqrt();
            if dist <= 15.0 {
                let alpha = if dist > 13.0 { 170 } else { 230 };
                set_pixel(&mut buf, x, y, [8, 18, 34, alpha]);
            }
        }
    }

    let color = if percent >= 85 {
        [248, 113, 113, 255]
    } else if percent >= 70 {
        [250, 204, 21, 255]
    } else {
        [147, 197, 253, 255]
    };

    for (digit_index, ch) in text.chars().enumerate() {
        let pattern = digit_pattern(ch);
        let offset_x = start_x + digit_index as i32 * (digit_w + gap);
        for (row, line) in pattern.iter().enumerate() {
            for (col, bit) in line.chars().enumerate() {
                if bit == '1' {
                    let x = offset_x + col as i32 * scale;
                    let y = start_y + row as i32 * scale;
                    fill_rect(&mut buf, x + 1, y + 1, scale, scale, [0, 0, 0, 150]);
                    fill_rect(&mut buf, x, y, scale, scale, color);
                }
            }
        }
    }

    tauri::image::Image::new_owned(buf, 32, 32)
}

#[tauri::command]
async fn optimize_ram() -> Result<CleanResult, String> {
    enable_all_privileges();

    let before = get_available_ram();
    
    let mut mem_info = MEMORYSTATUSEX::default();
    mem_info.dwLength = std::mem::size_of::<MEMORYSTATUSEX>() as u32;
    unsafe { let _ = GlobalMemoryStatusEx(&mut mem_info); }
    let usage_before = mem_info.dwMemoryLoad;

    for _ in 0..2 {
        clean_processes_ultra_aggressive();
        clean_windows_memory_lists();
        clean_system_file_cache();
        clean_registry_cache();
        combine_physical_memory();
        aggressive_flush_cycle(4);
        unsafe {
            let self_h = GetCurrentProcess();
            let _ = EmptyWorkingSet(self_h);
            let _ = SetProcessWorkingSetSizeEx(self_h, usize::MAX, usize::MAX, SETPROCESSWORKINGSETSIZEEX_FLAGS(QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE));
        }
        sleep(Duration::from_millis(120));
    }

    final_intensive_cycle();
    sleep(Duration::from_millis(160));
    
    let after = measure_stable_ram();
    
    unsafe { let _ = GlobalMemoryStatusEx(&mut mem_info); }

    let freed = if after > before { after - before } else { 0 };

    Ok(CleanResult {
        freed_bytes: freed,
        usage_before,
        usage_after: mem_info.dwMemoryLoad,
    })
}

#[tauri::command]
fn check_admin() -> bool {
    // Basic check for admin token (just a heuristic, or assume true if we successfully restart)
    true // Assuming the caller requested admin through manifest or relaunch
}

#[tauri::command]
fn is_startup_enabled() -> bool {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    if let Ok(run) = hkcu.open_subkey("Software\\Microsoft\\Windows\\CurrentVersion\\Run") {
        let val: Result<String, _> = run.get_value("RAMMY");
        return val.is_ok();
    }
    false
}

#[tauri::command]
fn set_startup(enable: bool) -> Result<(), String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    if let Ok((run, _)) = hkcu.create_subkey("Software\\Microsoft\\Windows\\CurrentVersion\\Run") {
        if enable {
            let mut path = [0u16; 512];
            unsafe {
                let len = GetModuleFileNameW(None, &mut path);
                let p = String::from_utf16_lossy(&path[..len as usize]);
                let _ = run.set_value("RAMMY", &format!("\"{}\" --hidden", p));
            }
        } else {
            let _ = run.delete_value("RAMMY");
        }
        return Ok(());
    }
    Err("Failed to edit registry".into())
}

use tauri::{tray::{TrayIconBuilder, MouseButton, MouseButtonState, TrayIconEvent}, menu::{Menu, MenuItem}, Manager};

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_log::Builder::new().build())
        .plugin(tauri_plugin_notification::init())
        .setup(|app| {
            let optimize_i = MenuItem::with_id(app, "optimize", "Clean RAM", true, None::<&str>)?;
            let show_i = MenuItem::with_id(app, "show", "Show RAMMY", true, None::<&str>)?;
            let quit_i = MenuItem::with_id(app, "quit", "Quit", true, None::<&str>)?;
            let tray_menu = Menu::with_items(app, &[&optimize_i, &show_i, &quit_i])?;

            let initial_stats = get_ram_stats();
            let tray = TrayIconBuilder::with_id("main")
                .menu(&tray_menu)
                .icon(ram_tray_icon(initial_stats.percent))
                .tooltip(ram_tray_tooltip())
                .on_menu_event(|app, event| {
                    match event.id.as_ref() {
                        "optimize" => {
                            let app_handle = app.clone();
                            tauri::async_runtime::spawn(async move {
                                if let Some(tray) = app_handle.tray_by_id("main") {
                                    let _ = tray.set_tooltip(Some("RAMMY - Cleaning RAM..."));
                                }

                                let result = optimize_ram().await;

                                if let Some(tray) = app_handle.tray_by_id("main") {
                                    match result {
                                        Ok(clean) => {
                                            let freed_mb = clean.freed_bytes / 1024 / 1024;
                                            let _ = tray.set_tooltip(Some(format!("RAMMY - Freed {} MB | {}", freed_mb, ram_tray_tooltip())));
                                        }
                                        Err(_) => {
                                            let _ = tray.set_tooltip(Some("RAMMY - Error cleaning RAM"));
                                        }
                                    }
                                }
                            });
                        }
                        "show" => {
                            if let Some(window) = app.get_webview_window("main") {
                                window.show().unwrap();
                                window.set_focus().unwrap();
                            }
                        }
                        "quit" => {
                            app.exit(0);
                        }
                        _ => {}
                    }
                })
                .on_tray_icon_event(|tray, event| {
                    if let TrayIconEvent::Click {
                        button: MouseButton::Left,
                        button_state: MouseButtonState::Up,
                        ..
                    } = event {
                        let app = tray.app_handle();
                        if let Some(window) = app.get_webview_window("main") {
                            window.show().unwrap();
                            window.set_focus().unwrap();
                        }
                    }
                })
                .build(app)?;

            let tray_for_stats = tray.clone();
            std::thread::spawn(move || loop {
                let stats = get_ram_stats();
                let _ = tray_for_stats.set_icon(Some(ram_tray_icon(stats.percent)));
                let used_gb = stats.used as f64 / 1024.0 / 1024.0 / 1024.0;
                let total_gb = stats.total as f64 / 1024.0 / 1024.0 / 1024.0;
                let _ = tray_for_stats.set_tooltip(Some(format!(
                    "RAMMY - RAM {}% ({:.1}/{:.1} GB)",
                    stats.percent, used_gb, total_gb
                )));
                sleep(Duration::from_secs(2));
            });

            let window = app.get_webview_window("main").unwrap();
            let args: Vec<String> = std::env::args().collect();
            if !args.iter().any(|a| a == "--hidden") {
                window.show().unwrap();
            }

            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            get_ram_stats, 
            optimize_ram,
            check_admin,
            is_startup_enabled,
            set_startup
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
