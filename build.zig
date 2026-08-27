const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const debug_build = b.option(bool, "debug", "debug build");
    const exe = b.addExecutable(.{
        .name = "computer_enhance",
        .use_llvm = debug_build,
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = debug_build,
        }),
    });
    exe.pie = debug_build;

    b.installArtifact(exe);

    const run_step = b.step("run", "Run the compiler");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);

    const install_on_run_cmd = b.addInstallArtifact(exe, .{});
    run_step.dependOn(&install_on_run_cmd.step);
}
