import PageBreadcrumb from "../components/common/PageBreadCrumb";
import DefaultInputs from "../components/form/form-elements/DefaultInputs";
import PageMeta from "../components/common/PageMeta";

export default function SystemSettings() {
  return (
    <div>
      <PageMeta title="System Settings" description="" />
      <PageBreadcrumb pageTitle="System Settings" />
      <div className="grid grid-cols-1 gap-6 xl:grid-cols-2">
        <div className="space-y-6">
          <DefaultInputs />
        </div>
      </div>
    </div>
  );
}
