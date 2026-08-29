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

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const arena = init.arena;
    const arena_alloc = arena.allocator();
    const listing_0039_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", "listing_0039_more_movs" };
    const listing_0039_path = try std.fs.path.join(arena_alloc, &listing_0039_paths);
    const content_listing_0039: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_0039_path, arena_alloc, .unlimited);

    var i: u32 = 0;
    while (i < content_listing_0039.len) {
        const instruction = content_listing_0039[i] >> 2;
        if (instruction == MOV_INSTRUCTION) {
            const direction_bit = (content_listing_0039[i] >> 1) & 0b00000001;
            const wide_bit = content_listing_0039[i] & 0b00000001;

            i += 1;
            const mod = content_listing_0039[i] >> 6;

            const reg_field = (content_listing_0039[i] >> 3) & 0b00000111;
            const reg_mem_field = content_listing_0039[i] & 0b00000111;

            // const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
            // const reg_mem_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_mem_field] else reg_bits_name_map[reg_mem_field];

            var disp: u16 = undefined;

            // no displacement (Except when R/M=110, then 16 bit displacement)
            if (mod == 0b000000) {
                const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
                const reg_mem_effective_calc = reg_mem_effective_calc_map[reg_mem_field];
                // TODO: handle direct address
                std.debug.assert(reg_mem_field != 0b110);
                if (direction_bit == 1) {
                    std.debug.print("mov {s}, [{s}]\n", .{ reg_field_name, reg_mem_effective_calc });
                } else {
                    std.debug.print("mov [{s}], {s}\n", .{ reg_mem_effective_calc, reg_field_name });
                }
            }
            // 8 bit displacement
            else if (mod == 0b000001) {
                i += 1;
                disp = content_listing_0039[i];

                const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
                const reg_mem_effective_calc = reg_mem_effective_calc_map[reg_mem_field];
                if (direction_bit == 1) {
                    std.debug.print("mov {s}, [{s} + {d}]\n", .{ reg_field_name, reg_mem_effective_calc, disp });
                } else {
                    std.debug.print("mov [{s} + {d}], {s}\n", .{ reg_mem_effective_calc, disp, reg_field_name });
                }
            }
            // 16 bit displacement
            else if (mod == 0b000010) {
                i += 1;
                const disp_lo = content_listing_0039[i];
                i += 1;
                const disp_high: u16 = content_listing_0039[i];
                disp = (disp_high << 8) | disp_lo;

                const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
                const reg_mem_effective_calc = reg_mem_effective_calc_map[reg_mem_field];
                if (direction_bit == 1) {
                    std.debug.print("mov {s}, [{s} + {d}]\n", .{ reg_field_name, reg_mem_effective_calc, disp });
                } else {
                    std.debug.print("mov [{s} + {d}], {s}\n", .{ reg_mem_effective_calc, disp, reg_field_name });
                }
            }
            // Register mode (no displacement)
            else {
                const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
                const reg_mem_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_mem_field] else reg_bits_name_map[reg_mem_field];
                if (direction_bit == 1) {
                    std.debug.print("mov {s}, {s}\n", .{ reg_field_name, reg_mem_field_name });
                } else {
                    std.debug.print("mov {s}, {s}\n", .{ reg_mem_field_name, reg_field_name });
                }
            }

            // std.debug.print("{b:0>8}\n", .{mod});
            i += 1;
        } else if ((instruction >> 2) == IMM_INSTRUCTION) {
            const wide_bit = (content_listing_0039[i] >> 3) & 0b00000001;
            const reg_field = content_listing_0039[i] & 0b00000111;
            // const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
            i += 1;
            if (wide_bit == 1) {
                const reg_field_name = reg_wide_bits_name_map[reg_field];
                const data_low = content_listing_0039[i];
                i += 1;
                const data_high: u16 = content_listing_0039[i];
                const data = (data_high << 8) | data_low;
                std.debug.print("mov {s}, {d}\n", .{ reg_field_name, data });
            } else {
                const reg_field_name = reg_bits_name_map[reg_field];
                const data = content_listing_0039[i];
                std.debug.print("mov {s}, {d}\n", .{ reg_field_name, data });
            }
            i += 1;
        }
    }
}
