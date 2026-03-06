<template>
  <div class="app-container flex flex-col h-full bg-slate-900 text-slate-200">
    <AppHeader />
    <div class="flex flex-1 overflow-hidden">
      <AppSidebar />
      <MainView />
    </div>
    <StatusBar />
  </div>
</template>

<script setup lang="ts">
import AppHeader from './components/layout/AppHeader.vue'
import AppSidebar from './components/layout/AppSidebar.vue'
import StatusBar from './components/layout/StatusBar.vue'
import MainView from './views/MainView.vue'
</script>
