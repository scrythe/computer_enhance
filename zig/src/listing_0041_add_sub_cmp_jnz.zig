const std = @import("std");
const Io = std.Io;
const Allocator = std.mem.Allocator;

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const arena = init.arena;
    const arena_alloc = arena.allocator();
    const listing_0041_paths = [_][]const u8{ "..", "computer_enhance", "perfaware", "part1", "listing_0041_add_sub_cmp_jnz" };
    const listing_0041_path = try std.fs.path.join(arena_alloc, &listing_0041_paths);
    const content_listing_0041: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_0041_path, arena_alloc, .unlimited);

    const longest_instruction_length = "add word [bx + si + 65535], 65535".len;
    const instructions_amount = content_listing_0041.len / 2;

    const disassembly_buffer = try arena_alloc.alloc(u8, instructions_amount * longest_instruction_length);

    const disassembly_slice = try disassembly_binary(disassembly_buffer, content_listing_0041);
    std.debug.print("{s}\n", .{disassembly_slice});
    try check_output(arena_alloc, io, content_listing_0041, disassembly_slice);
}

fn check_output(arena_alloc: Allocator, io: Io, content_listing_0041: []const u8, disassembly_slice: []const u8) !void {
    const listing_0040_disassembly_output_paths = [_][]const u8{ "testing_results", "listing_0041_add_sub_cmp_jnz.asm" };
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

    // show_binaries(content_listing_0040, nasm_result.stdout);

    try std.testing.expectEqualStrings(content_listing_0041, nasm_result.stdout);
}

// ADD
// to and from reg/mem 000000;
// Imm to reg/mem      100000;  mod 000 r/m
// Imm to acc          0000010;

// SUB
// to and from reg/mem 001010;
// Imm to reg/mem      100000;  mod 101 r/m
// Imm to acc          0010110;

// CMP
// to and from reg/mem 001110;
// Imm to reg/mem      100000;  mod 111 r/m
// Imm to acc          0011110;

// bitwise and the instruction with this constant to see if the relevant bits for the
// to and from reg/mem instruction are equal to 00xxx0xx where x can be
// anything (first 3 determines if add sub or cmp)
const TO_FROM_REG_MEM_RELEVANT_BITS = 0b11000100;
const TO_FROM_REG_MEM_INSTRUCT = 0b00000000;

// same concept as above, should match 00xxx10x;
const IMM_TO_ACC_RELEVANT_BITS = 0b11000110;
const IMM_TO_ACC_INSTRUCT = 0b00000100;

const IMM_TO_REG_INSTRUCT = 0b100000;

const oppcode_to_name_map = [8]*const [3]u8{
    "add", // 000
    "   ", // 001
    "   ", // 010
    "   ", // 011
    "   ", // 100
    "sub", // 101
    "   ", // 110
    "cmp", // 111
};

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

const ADD = 0b000;
const SUB = 0b101;
const CMP = 0b111;

fn disassembly_binary(disassembly_buffer: []u8, content_listing_0041: []u8) ![]u8 {
    var disassembly_buffer_writer = std.Io.Writer.fixed(disassembly_buffer);
    try disassembly_buffer_writer.print("; listing_0041_add_sub_cmp_jnz disassembly: \nbits 16", .{});
    var i: u32 = 0;
    var temp_print_buff: [1024]u8 = undefined;
    var temp_print_writer = std.Io.Writer.fixed(&temp_print_buff);
    while (i < content_listing_0041.len) {
        if ((content_listing_0041[i] & TO_FROM_REG_MEM_RELEVANT_BITS) == TO_FROM_REG_MEM_INSTRUCT) {
            const direction_bit = (content_listing_0041[i] >> 1) & 0b0000001;
            const wide_bit = content_listing_0041[i] & 0b00000001;

            const oppcode = (content_listing_0041[i] >> 3) & 0b000111;
            const oppcode_name = oppcode_to_name_map[oppcode];

            i += 1;
            const mod_field = content_listing_0041[i] >> 6;
            const reg_field = (content_listing_0041[i] >> 3) & 0b000111;
            const reg_mem_field = content_listing_0041[i] & 0b00000111;
            var reg_field_name: *const [2]u8 = undefined;
            var reg_mem_field_name: *const [2]u8 = undefined;
            const effective_addr_name = reg_mem_effective_calc_map[reg_mem_field];
            if (wide_bit == 1) {
                reg_field_name = reg_wide_bits_name_map[reg_field];
                reg_mem_field_name = reg_wide_bits_name_map[reg_mem_field];
            } else {
                reg_field_name = reg_bits_name_map[reg_field];
                reg_mem_field_name = reg_bits_name_map[reg_mem_field];
            }

            const second_field, i = try get_mod_based_field(
                &temp_print_writer,
                &temp_print_buff,
                content_listing_0041,
                mod_field,
                reg_mem_field,
                reg_mem_field_name,
                effective_addr_name,
                i,
            );

            if (direction_bit == 1) {
                try disassembly_buffer_writer.print("\n{s} {s}, {s}", .{ oppcode_name, reg_field_name, second_field });
            } else {
                try disassembly_buffer_writer.print("\n{s} {s}, {s}", .{ oppcode_name, second_field, reg_field_name });
            }

            i += 1;
        } else if ((content_listing_0041[i] >> 2) == IMM_TO_REG_INSTRUCT) {
            // const sign_extend_bit = (content_listing_0041[i] >> 1) & 0b0000001;
            const wide_bit = content_listing_0041[i] & 0b00000001;

            i += 1;
            const mod_field = content_listing_0041[i] >> 6;

            const oppcode = (content_listing_0041[i] >> 3) & 0b000111;
            const oppcode_name = oppcode_to_name_map[oppcode];

            const reg_mem_field = content_listing_0041[i] & 0b00000111;
            const effective_addr_name = reg_mem_effective_calc_map[reg_mem_field];
            var reg_mem_field_name: *const [2]u8 = undefined;
            var size: *const [4]u8 = undefined;
            if (wide_bit == 1) {
                reg_mem_field_name = reg_wide_bits_name_map[reg_mem_field];
                size = "word";
            } else {
                reg_mem_field_name = reg_bits_name_map[reg_mem_field];
                size = "byte";
            }

            const second_field, i = try get_mod_based_field(
                &temp_print_writer,
                &temp_print_buff,
                content_listing_0041,
                mod_field,
                reg_mem_field,
                reg_mem_field_name,
                effective_addr_name,
                i,
            );

            i += 1;
            const data_low = content_listing_0041[i];
            const data = data_low;
            // const data = if (wide_bit == 1) blk: {
            //     i += 1;
            //     const data_high: u16 = content_listing_0041[i];
            //     std.debug.print("hmmm, {d}\n", .{(data_high << 8) | data_low});
            //     break :blk (data_high << 8) | data_low;
            // } else data_low;
            try disassembly_buffer_writer.print("\n{s} {s} {s}, {d}", .{ oppcode_name, size, second_field, data });

            i += 1;
        } else if ((content_listing_0041[i] & IMM_TO_ACC_RELEVANT_BITS) == IMM_TO_ACC_INSTRUCT) {
            const wide_bit = content_listing_0041[i] & 0b00000001;

            const oppcode = (content_listing_0041[i] >> 3) & 0b000111;
            const oppcode_name = oppcode_to_name_map[oppcode];

            i += 1;
            const data_low = content_listing_0041[i];
            const data = if (wide_bit == 1) blk: {
                i += 1;
                const data_high: u16 = content_listing_0041[i];
                break :blk (data_high << 8) | data_low;
            } else data_low;

            if (wide_bit == 1) {
                try disassembly_buffer_writer.print("\n{s} ax, {d}", .{ oppcode_name, data });
            } else {
                try disassembly_buffer_writer.print("\n{s} al, {d}", .{ oppcode_name, data });
            }
            i += 1;
        } else {
            const jump_name = switch (content_listing_0041[i]) {
                0b01110100 => "je",
                0b01111100 => "jl",
                0b01111110 => "jle",
                0b01110010 => "jb",
                0b01110110 => "jbe",
                0b01111010 => "jp",
                0b01110000 => "jo",
                0b01111000 => "js",
                0b01110101 => "jne",
                0b01111101 => "jnl",
                0b01111111 => "jnle",
                0b01110011 => "jnb",
                0b01110111 => "jnbe",
                0b01111011 => "jnp",
                0b01110001 => "jno",
                0b01111001 => "jns",
                0b11100010 => "loop",
                0b11100001 => "loopz",
                0b11100000 => "loopnz",
                0b11100011 => "jcxz",
                else => break,
            };
            std.debug.print("a: {s}\n", .{jump_name});
            i += 1;
            // Thanks to @mmozeiko at substacks for the $+offset
            // https://open.substack.com/pub/computerenhance/p/opcode-patterns-in-8086-arithmetic?r=8mnmxa&utm_campaign=comment-list-share-cta&utm_medium=web&comments=true&commentId=13475922
            const jump_pos_offset: i8 = @as(i8, @bitCast(content_listing_0041[i])) + 2;
            const sign: u8 = if (jump_pos_offset < 0) '-' else '+';
            try disassembly_buffer_writer.print("\n{s} ${c}{d}", .{ jump_name, sign, @abs(jump_pos_offset) });

            i += 1;
        }
    }
    return disassembly_buffer[0..disassembly_buffer_writer.end];
}

fn get_mod_based_field(
    temp_print_writer: *std.Io.Writer,
    temp_print_buff: []u8,
    content_listing_0041: []const u8,
    mod_field: u8,
    reg_mem_field: u8,
    reg_mem_field_name: []const u8,
    effective_addr_name: []const u8,
    og_i: u32,
) !struct { []const u8, u32 } {
    var i = og_i;
    var second_field: []const u8 = undefined;
    switch (mod_field) {
        // no displacement (except when r/m=110, then 16 bit displacement)
        0b00 => {
            const start = temp_print_writer.end;
            if (reg_mem_field == 0b110) {
                i += 1;
                const disp_lo = content_listing_0041[i];
                i += 1;
                const disp_high: u16 = content_listing_0041[i];
                const disp = (disp_high << 8) | disp_lo;

                try temp_print_writer.print("[{d}]", .{disp});
            } else {
                try temp_print_writer.print("[{s}]", .{effective_addr_name});
            }
            second_field = temp_print_buff[start..temp_print_writer.end];
        },
        0b01 => {
            i += 1;
            const disp: i8 = @bitCast(content_listing_0041[i]);
            const sign: u8 = if (disp < 0) '-' else '+';

            const start = temp_print_writer.end;
            try temp_print_writer.print("[{s} {c} {d}]", .{ effective_addr_name, sign, @abs(disp) });
            second_field = temp_print_buff[start..temp_print_writer.end];
        },
        0b10 => {
            i += 1;
            const disp_lo = content_listing_0041[i];
            i += 1;
            const disp_high: u16 = content_listing_0041[i];
            const disp = (disp_high << 8) | disp_lo;

            const start = temp_print_writer.end;
            try temp_print_writer.print("[{s} + {d}]", .{ effective_addr_name, disp });
            second_field = temp_print_buff[start..temp_print_writer.end];
        },
        0b11 => {
            second_field = reg_mem_field_name;
        },
        else => unreachable,
    }
    return .{ second_field, i };
}
