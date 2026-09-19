const std = @import("std");
const Io = std.Io;
const Allocator = std.mem.Allocator;

const sim86 = @import("sim86");
const Instruction = sim86.Instruction;

const Flags = enum(u8) { C, P, A, S, Z, O };
const FlagsMap = blk: {
    const flags_fields = @typeInfo(Flags).@"enum".fields;
    var flags_map: [flags_fields.len]u8 = undefined;
    for (flags_fields, 0..) |flag, i| {
        flags_map[i] = flag.name[0];
    }
    break :blk flags_map;
};

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const arena = init.arena;
    const arena_alloc = arena.allocator();
    const listing_paths = [_][]const u8{ "..", "computer_enhance", "perfaware", "part1", "listing_0048_ip_register" };
    const listing_path = try std.fs.path.join(arena_alloc, &listing_paths);
    const listing_content: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_path, arena_alloc, .unlimited);

    const listing_expected_output_paths = [_][]const u8{ "..", "computer_enhance", "perfaware", "part1", "listing_0048_ip_register.txt" };
    const listing_expected_output_path = try std.fs.path.join(arena_alloc, &listing_expected_output_paths);
    const listing_expected_output: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_expected_output_path, arena_alloc, .unlimited);

    const listing_output_buffer = try arena_alloc.alloc(u8, listing_expected_output.len * 2);
    var listing_output_writer = std.Io.Writer.fixed(listing_output_buffer);

    var registers: [14]u16 = undefined;
    var ip_reg: u16 = 0;
    @memset(&registers, 0);
    var flags: [FlagsMap.len]bool = undefined;

    try execute_instructions(
        &listing_output_writer,
        &registers,
        &ip_reg,
        &flags,
        listing_content,
    );

    try listing_output_writer.print("\r\nFinal registers:\r\n", .{});
    for (registers, 0..) |register, reg_i| {
        if (register != 0) {
            var reg =
                sim86.RegisterAccess{
                    .Index = @intCast(reg_i + 1),
                    .Offset = 0,
                    .Count = 2,
                };
            const reg_name = sim86.registerNameFromOperand(&reg);
            try listing_output_writer.print("      {s}: 0x{x:0>4} ({d})\r\n", .{ reg_name, register, register });
        }
    }
    try listing_output_writer.print("      ip: 0x{x:0>4} ({d})\r\n", .{ ip_reg, ip_reg });
    try listing_output_writer.print("   flags: ", .{});
    for (flags, 0..) |flag_val, i| {
        if (flag_val) {
            try listing_output_writer.printAsciiChar(FlagsMap[i], .{});
        }
    }
    try listing_output_writer.print("\r\n", .{});

    try listing_output_writer.print("\r\n", .{});

    const listing_output = listing_output_buffer[0..listing_output_writer.end];

    try std.testing.expectEqualStrings(listing_expected_output, listing_output);
    std.debug.print("{s}", .{listing_output});
}

fn execute_instructions(listing_output_writer: *std.Io.Writer, registers: *[14]u16, ip_reg: *u16, flags: *[FlagsMap.len]bool, listing_content: []u8) !void {
    try listing_output_writer.print("--- test\\listing_0048_ip_register execution ---\r\n", .{});

    var temp_print_buffer: [1024]u8 = undefined;
    var temp_print_writer = std.Io.Writer.fixed(&temp_print_buffer);

    while (ip_reg.* < listing_content.len) {
        const decoded = try sim86.decode8086Instruction(listing_content[ip_reg.*..]);
        var dest_reg = decoded.Operands[0].data.Register;
        var dest_reg_word = sim86.RegisterAccess{ .Index = dest_reg.Index, .Offset = 0, .Count = 2 };
        const dest_reg_name = sim86.registerNameFromOperand(&dest_reg);
        const dest_reg_word_name = sim86.registerNameFromOperand(&dest_reg_word);

        const prev_dest_reg_word_data = registers[dest_reg.Index - 1];

        const prev_flags_name = blk: {
            const start = temp_print_writer.end;
            for (flags, 0..) |flag_val, i| {
                if (flag_val) {
                    try temp_print_writer.printAsciiChar(FlagsMap[i], .{});
                }
            }
            break :blk temp_print_buffer[start..temp_print_writer.end];
        };

        const prev_ip_reg_data = ip_reg.*;

        // two bytes
        var dest_reg_data = prev_dest_reg_word_data;
        // high byte
        if (dest_reg.Count == 1 and dest_reg.Offset == 0) {
            dest_reg_data = dest_reg_data >> 8;
        }
        // low byte
        if (dest_reg.Offset == 1) {
            dest_reg_data = dest_reg_data & 0x00FF;
        }

        const second_operand = decoded.Operands[1];
        var second_operand_data: u16 = undefined;
        var second_operand_name: []const u8 = undefined;

        if (second_operand.Type == .OperandRegister) {
            var second_operand_register = second_operand.data.Register;
            // two bytes
            second_operand_data = registers[second_operand_register.Index - 1];
            // high byte
            if (second_operand_register.Count == 1 and second_operand_register.Offset == 0) {
                second_operand_data = second_operand_data >> 8;
            }
            // low byte
            if (second_operand_register.Offset == 1) {
                second_operand_data = second_operand_data & 0x00FF;
            }
            second_operand_name = sim86.registerNameFromOperand(&second_operand_register);
        } else {
            const second_operand_data_u32: u32 = @bitCast(second_operand.data.Immediate.Value);
            second_operand_data = @truncate(second_operand_data_u32);
            const start = temp_print_writer.end;
            try temp_print_writer.printInt(second_operand.data.Immediate.Value, 10, .lower, .{});
            second_operand_name = temp_print_buffer[start..temp_print_writer.end];
        }

        var res: u16 = 0;
        switch (decoded.Op) {
            .Op_mov => {
                res = second_operand_data;
            },
            .Op_sub, .Op_cmp => {
                // -% is sub with wrapping
                const dest_reg_data_i16: i16 = @bitCast(dest_reg_data);
                const second_operand_data_i16: i16 = @bitCast(second_operand_data);
                const u_sub_res = @subWithOverflow(dest_reg_data, second_operand_data);
                const i_sub_res = @subWithOverflow(dest_reg_data_i16, second_operand_data_i16);
                res = @bitCast(i_sub_res[0]);
                flags[@intFromEnum(Flags.C)] = u_sub_res[1] == 1;
                flags[@intFromEnum(Flags.O)] = i_sub_res[1] == 1;
                flags[@intFromEnum(Flags.S)] = i_sub_res[0] < 0;
                flags[@intFromEnum(Flags.Z)] = i_sub_res[0] == 0;
                const number_of_bits_lower_reg = @popCount(res & 0x00FF);
                flags[@intFromEnum(Flags.P)] = number_of_bits_lower_reg % 2 == 0;

                const dest_reg_data_u4: u4 = @truncate(dest_reg_data);
                const second_operand_data_u4: u4 = @truncate(second_operand_data);
                // std.debug.print("{d} {d}\n", .{ dest_reg_data_u4, second_operand_data_u4 });
                const four_bitadd_res = @subWithOverflow(dest_reg_data_u4, second_operand_data_u4);
                flags[@intFromEnum(Flags.A)] = four_bitadd_res[1] == 1;
            },
            .Op_add => {
                const dest_reg_data_i16: i16 = @bitCast(dest_reg_data);
                const second_operand_data_i16: i16 = @bitCast(second_operand_data);
                const u_add_res = @addWithOverflow(dest_reg_data, second_operand_data);
                const i_add_res = @addWithOverflow(dest_reg_data_i16, second_operand_data_i16);
                res = @bitCast(i_add_res[0]);
                flags[@intFromEnum(Flags.C)] = u_add_res[1] == 1;
                flags[@intFromEnum(Flags.O)] = i_add_res[1] == 1;
                flags[@intFromEnum(Flags.S)] = i_add_res[0] < 0;
                flags[@intFromEnum(Flags.Z)] = i_add_res[0] == 0;
                const number_of_bits_lower_reg = @popCount(res & 0x00FF);
                flags[@intFromEnum(Flags.P)] = number_of_bits_lower_reg % 2 == 0;

                const dest_reg_data_u4: u4 = @truncate(dest_reg_data);
                const second_operand_data_u4: u4 = @truncate(second_operand_data);
                const four_bitadd_res = @addWithOverflow(dest_reg_data_u4, second_operand_data_u4);
                flags[@intFromEnum(Flags.A)] = four_bitadd_res[1] == 1;
            },
            else => {
                res = res;
            },
        }

        if (decoded.Op != .Op_cmp) {
            // two bytes
            if (dest_reg.Count == 2) {
                registers[dest_reg.Index - 1] = res;
                // high byte
            } else if (dest_reg.Offset == 0) {
                registers[dest_reg.Index - 1] = (registers[dest_reg.Index - 1] & 0xFF00) | res;
                // low byte
            } else {
                registers[dest_reg.Index - 1] = (registers[dest_reg.Index - 1] & 0x00FF) | (res << 8);
            }
        }

        const dest_reg_new_word_data = registers[dest_reg.Index - 1];
        var dest_reg_change_text: []const u8 = "";
        if (prev_dest_reg_word_data != dest_reg_new_word_data) {
            const start = temp_print_writer.end;
            try temp_print_writer.print(" {s}:0x{x}->0x{x}", .{ dest_reg_word_name, prev_dest_reg_word_data, dest_reg_new_word_data });
            dest_reg_change_text = temp_print_buffer[start..temp_print_writer.end];
        }

        const new_flags_name = blk: {
            const start = temp_print_writer.end;
            for (flags, 0..) |flag_val, i| {
                if (flag_val) {
                    try temp_print_writer.printAsciiChar(FlagsMap[i], .{});
                }
            }
            break :blk temp_print_buffer[start..temp_print_writer.end];
        };

        var flags_change_text: []const u8 = "";
        if (!std.mem.eql(u8, prev_flags_name, new_flags_name)) {
            const start = temp_print_writer.end;
            try temp_print_writer.print(" flags:{s}->{s}", .{ prev_flags_name, new_flags_name });
            flags_change_text = temp_print_buffer[start..temp_print_writer.end];
        }

        ip_reg.* += @intCast(decoded.Size);

        const new_ip_reg_data = ip_reg.*;

        const mnemonic = sim86.mnemonicFromOperationType(decoded.Op);
        try listing_output_writer.print("{s} {s}, {s} ;{s} ip:0x{x}->0x{x}{s} \r\n", .{
            mnemonic,
            dest_reg_name,
            second_operand_name,
            dest_reg_change_text,
            prev_ip_reg_data,
            new_ip_reg_data,
            flags_change_text,
        });
    }
}
