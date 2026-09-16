const std = @import("std");
const Io = std.Io;
const Allocator = std.mem.Allocator;

const sim86 = @import("sim86");
const Instruction = sim86.Instruction;

const CX_REG_INDEX = 3;

const MEMORY_SIZE = 2 << 15;

const Flags = enum(u8) { C, P, A, S, Z, O };
const FlagsMap = blk: {
    const flags_fields = @typeInfo(Flags).@"enum".fields;
    var flags_map: [flags_fields.len]u8 = undefined;
    for (flags_fields, 0..) |flag, i| {
        flags_map[i] = flag.name[0];
    }
    break :blk flags_map;
};

const file_name = "listing_0052_memory_add_loop";

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const arena = init.arena;
    const arena_alloc = arena.allocator();
    const listing_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", file_name };
    const listing_path = try std.fs.path.join(arena_alloc, &listing_paths);
    const listing_content: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_path, arena_alloc, .unlimited);

    const listing_expected_output_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", file_name ++ ".txt" };
    const listing_expected_output_path = try std.fs.path.join(arena_alloc, &listing_expected_output_paths);
    const listing_expected_output: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_expected_output_path, arena_alloc, .unlimited);

    const listing_output_buffer = try arena_alloc.alloc(u8, listing_expected_output.len * 2);
    var listing_output_writer = std.Io.Writer.fixed(listing_output_buffer);

    var memory: [MEMORY_SIZE]u8 = undefined;
    @memset(&memory, 0);
    var registers: [14]u16 = undefined;
    var ip_reg: u16 = 0;
    @memset(&registers, 0);
    var flags: [FlagsMap.len]bool = undefined;

    try execute_instructions(
        &listing_output_writer,
        &memory,
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

    var temp_print_buffer: [1024]u8 = undefined;
    var temp_print_writer = std.Io.Writer.fixed(&temp_print_buffer);

    const flags_text_start = temp_print_writer.end;
    for (flags, 0..) |flag_val, i| {
        if (flag_val) {
            try temp_print_writer.printAsciiChar(FlagsMap[i], .{});
        }
    }

    if (temp_print_writer.end > flags_text_start) {
        try listing_output_writer.print("   flags: {s}\r\n", .{temp_print_buffer[flags_text_start..temp_print_writer.end]});
    }

    try listing_output_writer.print("\r\n", .{});

    const listing_output = listing_output_buffer[0..listing_output_writer.end];

    try std.testing.expectEqualStrings(listing_expected_output, listing_output);
    std.debug.print("{s}", .{listing_output});
}

fn execute_instructions(
    listing_output_writer: *std.Io.Writer,
    memory: *[MEMORY_SIZE]u8,
    registers: *[14]u16,
    ip_reg: *u16,
    flags: *[FlagsMap.len]bool,
    listing_content: []u8,
) !void {
    try listing_output_writer.print("--- test\\{s} execution ---\r\n", .{file_name});

    var temp_print_buffer: [1024]u8 = undefined;
    var temp_print_writer = std.Io.Writer.fixed(&temp_print_buffer);

    while (ip_reg.* < listing_content.len) {
        const decoded = try sim86.decode8086Instruction(listing_content[ip_reg.*..]);
        const prev_ip_reg = ip_reg.*;
        // replaced later if jump Instruction
        var new_ip_reg: u16 = ip_reg.* + @as(u16, @truncate(decoded.Size));

        var prev_reg: u32 = 0; // Index, 0 if no reg
        var prev_reg_val: u16 = 0;

        var operands_text: [2][]const u8 = undefined;
        var operands_data: [2]u16 = undefined;
        var operands_effec_addr_data: [2]u16 = undefined;
        for (decoded.Operands, 0..) |operand, i| {
            switch (operand.Type) {
                .OperandRegister => {
                    var operand_reg = operand.data.Register;
                    operands_data[i] =
                        if (operand_reg.Count == 2)
                            // two bytes
                            registers[operand_reg.Index - 1]
                        else if (operand_reg.Offset == 0)
                            // low byte
                            registers[operand_reg.Index - 1] & 0x00FF
                        else
                            // high byte
                            registers[operand_reg.Index - 1] >> 8;

                    operands_text[i] = sim86.registerNameFromOperand(&operand_reg);
                },
                .OperandMemory => {
                    const operand_effec_addr = operand.data.Address;

                    const start = temp_print_writer.end;
                    try temp_print_writer.printAsciiChar('[', .{});

                    var term1_reg = operand_effec_addr.Terms[0].Register;
                    const term1_val = if (term1_reg.Index == 0) 0 else registers[term1_reg.Index - 1];
                    const term1_reg_name = sim86.registerNameFromOperand(&term1_reg);
                    try temp_print_writer.print("{s}", .{term1_reg_name});

                    var term2_reg = operand_effec_addr.Terms[1].Register;
                    const term2_val = if (term2_reg.Index == 0) 0 else registers[term2_reg.Index - 1];
                    const term2_reg_name = sim86.registerNameFromOperand(&term2_reg);
                    if (term2_reg_name.len > 0) {
                        try temp_print_writer.print("+{s}", .{term2_reg_name});
                    }

                    const displacement = operand_effec_addr.Displacement;
                    if (displacement != 0) {
                        try temp_print_writer.print("+{d}", .{displacement});
                    }

                    try temp_print_writer.printAsciiChar(']', .{});
                    operands_text[i] = temp_print_buffer[start..temp_print_writer.end];
                    const effect_addr_val = term1_val + term2_val + displacement;
                    operands_effec_addr_data[i] = @intCast(effect_addr_val);
                    const low = memory[operands_effec_addr_data[i]];
                    const high: u16 = memory[operands_effec_addr_data[i] + 1];
                    operands_data[i] = high << 8 | low;
                    // std.debug.print("low: {d}, high: {d}, full: {d}, addr: {d}\n", .{ low, high, operands_data[i], operands_effec_addr_data[i] });
                },
                .OperandImmediate => {
                    const start = temp_print_writer.end;
                    try temp_print_writer.printInt(operand.data.Immediate.Value, 10, .lower, .{});
                    operands_text[i] = temp_print_buffer[start..temp_print_writer.end];
                    const operands_data_i16: i16 = @truncate(operand.data.Immediate.Value);
                    operands_data[i] = @bitCast(operands_data_i16);
                },
                .OperandNone => {},
            }
        }

        var instruction_arguments_text: []const u8 = "";
        var registers_change_text: []const u8 = "";
        var flags_change_text: []const u8 = "";
        const prev_flags_name = blk: {
            const start = temp_print_writer.end;
            for (flags, 0..) |flag_val, i| {
                if (flag_val) {
                    try temp_print_writer.printAsciiChar(FlagsMap[i], .{});
                }
            }
            break :blk temp_print_buffer[start..temp_print_writer.end];
        };

        var res: u16 = undefined;
        switch (decoded.Op) {
            .Op_mov => {
                res = operands_data[1];
            },
            .Op_sub, .Op_cmp => {
                const operands_data_i16_1: i16 = @bitCast(operands_data[0]);
                const operands_data_i16_2: i16 = @bitCast(operands_data[1]);

                const signed_sub = @subWithOverflow(operands_data_i16_1, operands_data_i16_2);
                const unsigned_sub = @subWithOverflow(operands_data[0], operands_data[1]);

                flags[@intFromEnum(Flags.C)] = unsigned_sub[1] == 1;
                flags[@intFromEnum(Flags.O)] = signed_sub[1] == 1;
                flags[@intFromEnum(Flags.S)] = signed_sub[0] < 0;
                flags[@intFromEnum(Flags.Z)] = signed_sub[0] == 0;

                const number_of_bits_lower_reg = @popCount(signed_sub[0] & 0x00FF);
                flags[@intFromEnum(Flags.P)] = number_of_bits_lower_reg % 2 == 0;

                const operands_data_u4_1: u4 = @truncate(operands_data[0]);
                const operands_data_u4_2: u4 = @truncate(operands_data[1]);

                const unsigned_sub_u4 = @subWithOverflow(operands_data_u4_1, operands_data_u4_2);
                flags[@intFromEnum(Flags.A)] = unsigned_sub_u4[1] == 1;

                res = @bitCast(signed_sub[0]);
            },
            .Op_add => {
                const operands_data_i16_1: i16 = @bitCast(operands_data[0]);
                const operands_data_i16_2: i16 = @bitCast(operands_data[1]);

                const signed_add = @addWithOverflow(operands_data_i16_1, operands_data_i16_2);
                const unsigned_add = @addWithOverflow(operands_data[0], operands_data[1]);

                flags[@intFromEnum(Flags.C)] = unsigned_add[1] == 1;
                flags[@intFromEnum(Flags.O)] = signed_add[1] == 1;
                flags[@intFromEnum(Flags.S)] = signed_add[0] < 0;
                flags[@intFromEnum(Flags.Z)] = signed_add[0] == 0;

                const number_of_bits_lower_reg = @popCount(signed_add[0] & 0x00FF);
                flags[@intFromEnum(Flags.P)] = number_of_bits_lower_reg % 2 == 0;

                const operands_data_u4_1: u4 = @truncate(operands_data[0]);
                const operands_data_u4_2: u4 = @truncate(operands_data[1]);

                const unsigned_add_u4 = @addWithOverflow(operands_data_u4_1, operands_data_u4_2);
                flags[@intFromEnum(Flags.A)] = unsigned_add_u4[1] == 1;

                res = @bitCast(signed_add[0]);
            },
            .Op_jne => {
                if (!flags[@intFromEnum(Flags.Z)]) {
                    const offset_i16: i16 = @truncate(decoded.Operands[0].data.Immediate.Value);
                    const new_ip_reg_i16: i16 = @intCast(new_ip_reg);
                    new_ip_reg = @bitCast(new_ip_reg_i16 + offset_i16);
                }
            },
            .Op_je => {
                if (flags[@intFromEnum(Flags.Z)]) {
                    const offset_i16: i16 = @truncate(decoded.Operands[0].data.Immediate.Value);
                    const new_ip_reg_i16: i16 = @intCast(new_ip_reg);
                    new_ip_reg = @bitCast(new_ip_reg_i16 + offset_i16);
                }
            },
            .Op_jb => {
                if (flags[@intFromEnum(Flags.C)]) {
                    const offset_i16: i16 = @truncate(decoded.Operands[0].data.Immediate.Value);
                    const new_ip_reg_i16: i16 = @intCast(new_ip_reg);
                    new_ip_reg = @bitCast(new_ip_reg_i16 + offset_i16);
                }
            },
            .Op_loopnz => {
                prev_reg = CX_REG_INDEX;
                prev_reg_val = registers[CX_REG_INDEX - 1];
                registers[CX_REG_INDEX - 1] = @bitCast(@as(i16, @bitCast(registers[CX_REG_INDEX - 1])) - 1);
                if (registers[CX_REG_INDEX - 1] != 0 and !flags[@intFromEnum(Flags.Z)]) {
                    const offset_i16: i16 = @truncate(decoded.Operands[0].data.Immediate.Value);
                    const new_ip_reg_i16: i16 = @intCast(new_ip_reg);
                    new_ip_reg = @bitCast(new_ip_reg_i16 + offset_i16);
                }
            },
            .Op_jp => {
                if (flags[@intFromEnum(Flags.P)]) {
                    const offset_i16: i16 = @truncate(decoded.Operands[0].data.Immediate.Value);
                    const new_ip_reg_i16: i16 = @intCast(new_ip_reg);
                    new_ip_reg = @bitCast(new_ip_reg_i16 + offset_i16);
                }
            },
            else => {
                const mnemonic = sim86.mnemonicFromOperationType(decoded.Op);
                std.debug.print("not implemented: {s}\n", .{mnemonic});
                unreachable;
            },
        }

        switch (decoded.Op) {
            .Op_mov,
            .Op_add,
            .Op_sub,
            => {
                switch (decoded.Operands[0].Type) {
                    .OperandRegister => {
                        const reg = decoded.Operands[0].data.Register;
                        prev_reg = reg.Index;
                        prev_reg_val = registers[reg.Index - 1];
                        if (reg.Count == 2) {
                            // two bytes
                            registers[reg.Index - 1] = res;
                        } else if (reg.Offset == 0) {
                            // low byte
                            registers[reg.Index - 1] = (registers[reg.Index - 1] & 0xFF00) | res;
                        } else {
                            // high byte
                            registers[reg.Index - 1] = (registers[reg.Index - 1] & 0x00FF) | (res << 8);
                        }
                    },
                    .OperandMemory => {
                        // std.debug.print("{d}\n", .{operands_effec_addr_data[0]});
                        memory[operands_effec_addr_data[0]] = @truncate(res);
                        if (decoded.Flags.Wide) {
                            memory[operands_effec_addr_data[0] + 1] = @truncate(res << 8);
                        }
                    },
                    else => {},
                }
            },
            else => {},
        }

        switch (decoded.Op) {
            .Op_mov,
            .Op_sub,
            .Op_cmp,
            .Op_add,
            => {
                const start = temp_print_writer.end;

                if (decoded.Operands[0].Type == .OperandMemory) {
                    try temp_print_writer.print("word ", .{});
                }

                try temp_print_writer.print("{s}, {s}", .{
                    operands_text[0],
                    operands_text[1],
                });
                instruction_arguments_text = temp_print_buffer[start..temp_print_writer.end];
            },
            .Op_jne, .Op_je, .Op_jb, .Op_loopnz, .Op_jp => {
                const start = temp_print_writer.end;
                const offset = decoded.Operands[0].data.Immediate.Value + @as(i32, @intCast(decoded.Size));
                const offset_sign: u8 = if (offset < 0) '-' else '+';
                try temp_print_writer.print("${c}{d}", .{ offset_sign, @abs(offset) });
                instruction_arguments_text = temp_print_buffer[start..temp_print_writer.end];
            },
            else => {
                std.debug.print("not implemented: {any}\n", .{decoded.Op});
                unreachable;
            },
        }

        if (prev_reg != 0) {
            var reg_word = sim86.RegisterAccess{ .Index = prev_reg, .Offset = 0, .Count = 2 };
            const new_reg_val = registers[prev_reg - 1];

            if (prev_reg_val != new_reg_val) {
                const reg_name = sim86.registerNameFromOperand(&reg_word);
                const start = temp_print_writer.end;
                try temp_print_writer.print(" {s}:0x{x}->0x{x}", .{ reg_name, prev_reg_val, new_reg_val });
                registers_change_text = temp_print_buffer[start..temp_print_writer.end];
            }
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

        if (!std.mem.eql(u8, prev_flags_name, new_flags_name)) {
            const start = temp_print_writer.end;
            try temp_print_writer.print(" flags:{s}->{s}", .{ prev_flags_name, new_flags_name });
            flags_change_text = temp_print_buffer[start..temp_print_writer.end];
        }

        const mnemonic = sim86.mnemonicFromOperationType(decoded.Op);
        try listing_output_writer.print("{s} {s} ;{s} ip:0x{x}->0x{x}{s} \r\n", .{
            mnemonic,
            instruction_arguments_text,
            registers_change_text,
            prev_ip_reg,
            new_ip_reg,
            flags_change_text,
        });

        temp_print_writer.end = 0;

        ip_reg.* = @intCast(new_ip_reg);
    }
}
