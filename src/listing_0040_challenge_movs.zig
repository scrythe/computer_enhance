const std = @import("std");
const Io = std.Io;

const reg_bits_name_map = [8]*const [2]u8{
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh",
};
const reg_wide_bits_name_map = [8]*const [2]u8{
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di",
};
const reg_mem_effective_calc_map = [8][]const u8{
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx",
};
const MOV_INSTRUCTION = 0b00100010;
const IMM_INSTRUCTION = 0b00001011;
const IMM_REG_MEM_INSTRUCTION = 0b01100011;
const MEM_TO_ACC_INSTRUCTION = 0b01010000;
const ACC_TO_MEM_INSTRUCTION = 0b01010001;

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const arena = init.arena;
    const arena_alloc = arena.allocator();
    const listing_0040_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", "listing_0040_challenge_movs" };
    const listing_0040_path = try std.fs.path.join(arena_alloc, &listing_0040_paths);
    const content_listing_0040: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_0040_path, arena_alloc, .unlimited);
    const max_chars_per_instruction = "mov bx, [bx + si + 65535]".len;
    // could maybe less size since above instruction actually takes 4 bytes but also maybe some combination
    // could make it buffer overflow?
    const disassembly_buffer = try arena_alloc.alloc(u8, content_listing_0040.len / 2 * max_chars_per_instruction);
    const disassembly_slice = try disassembly_binary(disassembly_buffer, content_listing_0040);
    std.debug.print("{s}\n", .{disassembly_slice});

    const listing_0040_disassembly_output_paths = [_][]const u8{ "testing_results", "listing_0040_challenge_movs_disassembly.asm" };
    const listing_0040_disassembly_output_path = try std.fs.path.join(arena_alloc, &listing_0040_disassembly_output_paths);
    try std.Io.Dir.cwd().createDirPath(io, "testing_results");
    const file_listing_0039_disassembly = try std.Io.Dir.cwd().createFile(io, listing_0040_disassembly_output_path, .{ .truncate = true });
    try file_listing_0039_disassembly.writeStreamingAll(io, disassembly_slice);

    const nasm_result = try std.process.run(arena_alloc, io, .{
        .argv = &.{ "nasm", listing_0040_disassembly_output_path, "-o", "/dev/stdout" },
    });

    if (nasm_result.stderr.len > 0) {
        std.debug.print("{s}", .{nasm_result.stderr});
        return;
    }

    show_binaries(content_listing_0040, nasm_result.stdout);

    try std.testing.expectEqualStrings(content_listing_0040, nasm_result.stdout);
}

fn show_binaries(content_listing_0040: []u8, nasm_result: []u8) void {
    // var err_at_byte_i = undefined;
    std.debug.print("content_listing_0040\n", .{});
    var content_listing_byte_i: u32 = 0;
    // starts the byte itself at new line after 18 bytes, instead of relying on
    // terminal line wrapping at char, could be made dynamic tho but idc
    while (content_listing_byte_i < content_listing_0040.len) {
        const wrap_length = @min(content_listing_byte_i + 18, content_listing_0040.len);
        while (content_listing_byte_i < wrap_length) {
            const act_byte = content_listing_0040[content_listing_byte_i];
            std.debug.print("{b:0>8} ", .{act_byte});
            content_listing_byte_i += 1;
        }
        std.debug.print("\n", .{});
    }
    std.debug.print("nasm_result\n", .{});
    var nasm_byte_i: u32 = 0;
    while (nasm_byte_i < nasm_result.len) {
        const wrap_length = @min(nasm_byte_i + 18, nasm_result.len);
        while (nasm_byte_i < wrap_length) {
            const act_byte = nasm_result[nasm_byte_i];
            std.debug.print("{b:0>8} ", .{act_byte});
            nasm_byte_i += 1;
        }
        std.debug.print("\n", .{});
    }
}

/// many cases not properly handled and the code can be done way better but I
/// also don't really care too much atp
fn disassembly_binary(disassembly_buffer: []u8, content_listing_0040: []u8) ![]u8 {
    var disassembly_buffer_writer = std.Io.Writer.fixed(disassembly_buffer);

    try disassembly_buffer_writer.print("; listing_0040_challenge_movs disassembly:\nbits 16", .{});
    var i: u32 = 0;
    while (i < content_listing_0040.len) {
        if ((content_listing_0040[i] >> 2) == MOV_INSTRUCTION) {
            const direction_bit = (content_listing_0040[i] >> 1) & 0b00000001;
            const wide_bit = content_listing_0040[i] & 0b00000001;

            i += 1;
            const mod = content_listing_0040[i] >> 6;

            const reg_field = (content_listing_0040[i] >> 3) & 0b00000111;
            const reg_mem_field = content_listing_0040[i] & 0b00000111;

            const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
            const reg_mem_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_mem_field] else reg_bits_name_map[reg_mem_field];
            const reg_mem_effective_calc_type = reg_mem_effective_calc_map[reg_mem_field];

            // var disp: u16 = undefined;

            // no displacement (Except when R/M=110, then 16 bit displacement)
            if (mod == 0b000000) {
                if (reg_mem_field == 0b110) {}
                if (direction_bit == 1) {
                    // e.g.: mov cx, [bx + si]
                    try disassembly_buffer_writer.print("\nmov {s}, [", .{reg_field_name});
                    if (reg_mem_field == 0b110) {
                        i += 1;
                        const disp_lo = content_listing_0040[i];
                        i += 1;
                        const disp_high: u16 = content_listing_0040[i];
                        const disp = (disp_high << 8) | disp_lo;

                        try disassembly_buffer_writer.print("{d}]", .{disp});
                    } else {
                        try disassembly_buffer_writer.print("{s}]", .{reg_mem_effective_calc_type});
                    }
                } else {
                    // e.g.: mov [bx + si], cx
                    try disassembly_buffer_writer.print("\nmov [", .{});
                    if (reg_mem_field == 0b110) {
                        i += 1;
                        const disp_lo = content_listing_0040[i];
                        i += 1;
                        const disp_high: u16 = content_listing_0040[i];
                        const disp = (disp_high << 8) | disp_lo;

                        try disassembly_buffer_writer.print("{d}", .{disp});
                    } else {
                        try disassembly_buffer_writer.print("{s}", .{reg_mem_effective_calc_type});
                    }
                    try disassembly_buffer_writer.print("], {s}", .{reg_field_name});
                }
            }
            // 8 bit displacement
            else if (mod == 0b000001) {
                i += 1;
                const disp: i8 = @bitCast(content_listing_0040[i]);
                const sign: u8 = if (disp < 0) '-' else '+';

                if (direction_bit == 1) {
                    try disassembly_buffer_writer.print("\nmov {s}, [{s} {c} {d}]", .{
                        reg_field_name,
                        reg_mem_effective_calc_type,
                        sign,
                        @abs(disp),
                    });
                } else {
                    try disassembly_buffer_writer.print("\nmov [{s} {c} {d}], {s}", .{
                        reg_mem_effective_calc_type,
                        sign,
                        @abs(disp),
                        reg_field_name,
                    });
                }
            }
            // 16 bit displacement
            else if (mod == 0b000010) {
                i += 1;
                const disp_lo = content_listing_0040[i];
                i += 1;
                const disp_high: u16 = content_listing_0040[i];
                const disp = (disp_high << 8) | disp_lo;

                if (direction_bit == 1) {
                    try disassembly_buffer_writer.print("\nmov {s}, [{s} + {d}]", .{ reg_field_name, reg_mem_effective_calc_type, disp });
                } else {
                    try disassembly_buffer_writer.print("\nmov [{s} + {d}], {s}", .{ reg_mem_effective_calc_type, disp, reg_field_name });
                }
            }
            // Register mode (no displacement)
            else {
                if (direction_bit == 1) {
                    try disassembly_buffer_writer.print("\nmov {s}, {s}", .{ reg_field_name, reg_mem_field_name });
                } else {
                    try disassembly_buffer_writer.print("\nmov {s}, {s}", .{ reg_mem_field_name, reg_field_name });
                }
            }

            i += 1;
        } else if ((content_listing_0040[i] >> 4) == IMM_INSTRUCTION) {
            const wide_bit = (content_listing_0040[i] >> 3) & 0b00000001;
            const reg_field = content_listing_0040[i] & 0b00000111;
            // const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
            i += 1;
            if (wide_bit == 1) {
                const reg_field_name = reg_wide_bits_name_map[reg_field];
                const data_low = content_listing_0040[i];
                i += 1;
                const data_high: u16 = content_listing_0040[i];
                const data = (data_high << 8) | data_low;
                try disassembly_buffer_writer.print("\nmov {s}, {d}", .{ reg_field_name, data });
            } else {
                const reg_field_name = reg_bits_name_map[reg_field];
                const data = content_listing_0040[i];
                try disassembly_buffer_writer.print("\nmov {s}, {d}", .{ reg_field_name, data });
            }
            i += 1;
        } else if ((content_listing_0040[i] >> 1) == IMM_REG_MEM_INSTRUCTION) {
            const wide_bit = content_listing_0040[i] & 0b00000001;
            i += 1;

            const mod = content_listing_0040[i] >> 6;

            const reg_mem_field = content_listing_0040[i] & 0b00000111;
            const reg_mem_effective_calc_type = reg_mem_effective_calc_map[reg_mem_field];
            var disp: u16 = undefined;

            // no displacement (Except when R/M=110, then 16 bit displacement)
            if (mod == 0b000000) {
                i += 1;
                const data_low = content_listing_0040[i];
                const data = if (wide_bit == 1) blk: {
                    i += 1;
                    const data_high: u16 = content_listing_0040[i];
                    break :blk (data_high << 8) | data_low;
                } else data_low;
                // NOTE: skipping direct addresss here
                std.debug.assert(reg_mem_field != 0b110);
                try disassembly_buffer_writer.print("\nmov [{s}], byte {d}", .{ reg_mem_effective_calc_type, data });
            }
            // 8 bit displacement
            else if (mod == 0b000001) {
                i += 1;
                disp = content_listing_0040[i];

                i += 1;
                const data_low = content_listing_0040[i];
                const data = if (wide_bit == 1) blk: {
                    i += 1;
                    const data_high: u16 = content_listing_0040[i];
                    break :blk (data_high << 8) | data_low;
                } else data_low;

                try disassembly_buffer_writer.print("\nmov [{s} + {d}], byte {d}", .{ reg_mem_effective_calc_type, disp, data });
            }
            // 16 bit displacement
            else if (mod == 0b000010) {
                i += 1;
                const disp_lo = content_listing_0040[i];
                i += 1;
                const disp_high: u16 = content_listing_0040[i];
                disp = (disp_high << 8) | disp_lo;

                i += 1;
                const data_low = content_listing_0040[i];
                const data = if (wide_bit == 1) blk: {
                    i += 1;
                    const data_high: u16 = content_listing_0040[i];
                    break :blk (data_high << 8) | data_low;
                } else data_low;

                try disassembly_buffer_writer.print("\nmov [{s} + {d}], word {d}", .{ reg_mem_effective_calc_type, disp, data });
            }

            i += 1;
        } else if (content_listing_0040[i] >> 1 == MEM_TO_ACC_INSTRUCTION) {
            // always high even if not wide? so wide useless herer?
            // const wide_bit = content_listing_0040[i] & 0b00000001;
            i += 1;
            const addr_lo = content_listing_0040[i];
            i += 1;
            const addr_hi: u16 = content_listing_0040[i];
            const addr = (addr_hi << 8) | addr_lo;
            try disassembly_buffer_writer.print("\nmov ax, [{d}]", .{addr});
            i += 1;
        } else if (content_listing_0040[i] >> 1 == ACC_TO_MEM_INSTRUCTION) {
            // always high even if not wide? so wide useless herer?
            // const wide_bit = content_listing_0040[i] & 0b00000001;
            i += 1;
            const addr_lo = content_listing_0040[i];
            i += 1;
            const addr_hi: u16 = content_listing_0040[i];
            const addr = (addr_hi << 8) | addr_lo;
            try disassembly_buffer_writer.print("\nmov [{d}], ax", .{addr});
            i += 1;
        }
    }
    return disassembly_buffer[0..disassembly_buffer_writer.end];
}
